#include "Kernel.h"
#include "Instr.h"
#include "Reg.h"
#include "Flag.h"

#include <iostream>
#include <glob.h>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Linker.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/IPO.h"

using namespace std;
using namespace llvm;

Kernel::Kernel(const string& dir)
  : _dir(dir), _m(new Module(dir, getGlobalContext()))
{ }

bool Kernel::build()
{
  _parseCPUDefinition();

  for (uint32_t width = 8; width <= 32; width *= 2) {
    Type* type = Type::getIntNTy(getGlobalContext(), width);

    _buildFetchFunction(type);
    _buildWriteFunction(type);
  }

  _parseInstructions();
  _buildInstrExecuter();
  _buildInstrDisassembler();
  _link();

  if (verifyModule(*_m)) {
    cerr << "Verify failed" << endl;
    exit(1);
  }
  // return true;

  PassManager manager;
  FunctionPassManager fmanager(_m);
  PassManagerBuilder builder;

  vector<const char*> exportedFunctions;
  // callbacks don't neet to be exported expliziltly as there are still unresolved
  vector<string> exportedPrefixes = { "kernel" };

  for (Module::iterator f = _m->begin(), fe = _m->end(); f != fe; ++f) {
    for (auto p = exportedPrefixes.begin(), pe = exportedPrefixes.end(); p != pe; ++p) {
      if (f->getName().str().compare(0, (*p).size(), *p) == 0) {
        string s_str = f->getName().str();
        char* str = new char[s_str.length() + 1];
        strcpy(str, s_str.c_str());
        exportedFunctions.push_back(str);
      }
    }
  }

  manager.add(createInternalizePass(exportedFunctions));

  builder.OptLevel = 3;
  builder.BBVectorize = true;
  builder.SLPVectorize = true;
  builder.LoopVectorize = true;
  builder.RerollLoops = true;
  builder.populateFunctionPassManager(fmanager);
  builder.populateModulePassManager(manager);
  builder.populateLTOPassManager(manager, false, true);

  fmanager.doInitialization();
  for (Module::iterator F = _m->begin(), E = _m->end(); F != E; ++F)
    fmanager.run(*F);
  fmanager.doFinalization();

  manager.run(*_m);

  return true;
}

bool Kernel::_parseCPUDefinition()
{
  string filename = _dir + "/cpu.ll";

  cout << "Load " << filename << endl;

  LLVMContext& context = getGlobalContext();
  SMDiagnostic err;
  Module* m = ParseIRFile(filename, err, context);

  if (!m) {
    string what;
    raw_string_ostream os(what);
    err.print("", os);
    os.flush();
    cerr << what;
    return false;
  }

  _cpuModule = m;

  _parseRegDefs();

  _flags = Flag::parseModule(this, m);

  return true;
}

bool Kernel::_parseRegDefs()
{
  _regs = Reg::parseModule(_cpuModule);

  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  {
    Function* func;

    {
      vector<llvm::Type *> args;
      args.push_back(builder.getInt64Ty());

      FunctionType *funcType = FunctionType::get(builder.getInt8Ty()->getPointerTo(), args, false);
      func =
        Function::Create(funcType, Function::ExternalLinkage, "kernel.reg2str", _m);
      BasicBlock *entry = BasicBlock::Create(context, "", func);
      builder.SetInsertPoint(entry);
    }

    Value* reg = func->arg_begin();
    reg->setName("reg");

    BasicBlock *defaultBlock = BasicBlock::Create(context, "", func);

    SwitchInst* switchInstr = builder.CreateSwitch(reg, defaultBlock, _regs.size());

    for (auto r = _regs.begin(), e = _regs.end(); r != e; ++r) {
      string name = r->first;
      Reg* reg = r->second;

      if (!reg->isRealReg())
        continue;

      BasicBlock* block = BasicBlock::Create(context, "", func);
      switchInstr->addCase(builder.getInt64(reg->getNum()), block);
      builder.SetInsertPoint(block);
      builder.CreateRet(builder.CreateGlobalStringPtr(name));
    }

    builder.SetInsertPoint(defaultBlock);
    builder.CreateRet(builder.CreateGlobalStringPtr("unkown reg"));
  }

  {
    Function* targetFunc = _cpuModule->getFunction("cpu.reg.read");
    targetFunc = dyn_cast<Function>(_m->getOrInsertFunction(targetFunc->getName(), targetFunc->getFunctionType()));

    Function* func = Function::Create(targetFunc->getFunctionType(), Function::ExternalLinkage, "kernel.read_reg", _m);
    BasicBlock* block = BasicBlock::Create(context, "", func);
    builder.SetInsertPoint(block);
    vector<Value*> args;
    for (auto a = func->arg_begin(), e = func->arg_end(); a != e; ++a)
      args.push_back(a);
    Value* val = builder.CreateCall(targetFunc, args);
    builder.CreateRet(val);
  }

  {
    Function* targetFunc = _cpuModule->getFunction("cpu.reg.write");
    targetFunc = dyn_cast<Function>(_m->getOrInsertFunction(targetFunc->getName(), targetFunc->getFunctionType()));

    Function* func = Function::Create(targetFunc->getFunctionType(), Function::ExternalLinkage, "kernel.write_reg", _m);
    BasicBlock* block = BasicBlock::Create(context, "", func);
    builder.SetInsertPoint(block);
    vector<Value*> args;
    for (auto a = func->arg_begin(), e = func->arg_end(); a != e; ++a)
      args.push_back(a);
    builder.CreateCall(targetFunc, args);
    builder.CreateRetVoid();
  }

  return true;
}

bool Kernel::_parseInstructions()
{
  glob_t glob_result;
  string pattern = _dir + "/instr/*.ll";

  if (glob(pattern.c_str(), 0, NULL, &glob_result) != 0) {
    cerr << "Did not find any instruction files" << endl;
    return false;
  }

  for(unsigned int i=0;i<glob_result.gl_pathc;++i){
    _parseInstructionFile(glob_result.gl_pathv[i]);
  }

  globfree(&glob_result);
  return true;
}

Module* Kernel::parseInstrFile(string filename, SMDiagnostic &Err, LLVMContext &Context)
{
  OwningPtr<MemoryBuffer> File;
  if (llvm::error_code ec = MemoryBuffer::getFileOrSTDIN(filename, File)) {
    Err = SMDiagnostic(filename, SourceMgr::DK_Error,
                       "Could not open input file: " + ec.message());
    return 0;
  }

  std::unique_ptr<Module> M2(new Module(File->getBufferIdentifier(), Context));

  // Declare modules from cpu in this part
  for (Module::iterator f = _cpuModule->begin(), e = _cpuModule->end(); f != e; ++f) {
    M2->getOrInsertFunction(f->getName(), f->getFunctionType());
  }

  for (Module::iterator f = _m->begin(), e = _m->end(); f != e; ++f) {
    M2->getOrInsertFunction(f->getName(), f->getFunctionType());
  }

  return ParseAssembly(File.take(), M2.release(), Err, Context);
}

bool Kernel::_parseInstructionFile(const string& filename)
{
  cout << "Load " << filename << endl;

  LLVMContext& context = getGlobalContext();
  SMDiagnostic err;
  Module* m = parseInstrFile(filename, err, context);

  if (!m) {
    string what;
    raw_string_ostream os(what);
    err.print("", os);
    os.flush();
    cout << what;
    exit(1);
    return false;
  }

  auto instrs = Instr::parseModule(this, m);

  _instrs.insert(_instrs.end(), instrs.begin(), instrs.end());
  _instrModules.push_back(m);

  return true;
}

Function* Kernel::_buildInstrFunction(const string name, bool disassembler)
{
  LLVMContext & context = llvm::getGlobalContext();
  Module* _m = _cpuModule;
  IRBuilder<> builder(context);

  FunctionType *funcType;

  if (disassembler) {
    FunctionType* printFunctionType;
    {
      vector<llvm::Type *> args;
      args.push_back(builder.getInt8Ty()->getPointerTo());

      printFunctionType = FunctionType::get(builder.getVoidTy(), args, true);
    }

    vector<llvm::Type *> args;
    args.push_back(printFunctionType->getPointerTo());

    funcType = FunctionType::get(builder.getVoidTy(), args, false);
  }
  else {
    vector<llvm::Type *> args;

    funcType = FunctionType::get(builder.getVoidTy(), args, false);
  }
  Function *func =
    Function::Create(funcType, Function::ExternalLinkage, name, _m);
  BasicBlock* mainBlock = BasicBlock::Create(context, "", func);
  BasicBlock* exitBlock = BasicBlock::Create(context, "exit", func);
  BasicBlock* unkownBlock = BasicBlock::Create(context, "unkown", func);

  Function::arg_iterator args2 = func->arg_begin();
  Value* print;

  if (disassembler) {
    print = args2++;
    print->setName("print");
  }

  builder.SetInsertPoint(mainBlock);

  // Fetch the instruction
  Function* fetchInstr = _m->getFunction("cpu.fetch.instr");
  if (!fetchInstr) {
    cerr << "cpu.fetch.instr missing" << endl;
    exit(1);
  }
  Value* oldPC = builder.CreateCall(getNamedReg("PC")->getReadFunction());
  Value* res = builder.CreateCall(fetchInstr);
  Value* opcode = builder.CreateExtractValue(res, 0);
  Value* bits = builder.CreateExtractValue(res, 1);

  if (disassembler) {
    builder.CreateCall2(print, builder.CreateGlobalStringPtr("%8x: "), builder.CreateZExtOrTrunc(opcode, builder.getInt32Ty()));
  }

  SwitchInst* switchInst = builder.CreateSwitch(bits, exitBlock, 6);

  for (uint8_t width = 1; width <= 64; width *= 2) {
    BasicBlock* block = BasicBlock::Create(context, "", func);
    builder.SetInsertPoint(block);

    Value* opcodeTrunc = builder.CreateTrunc(opcode, Type::getIntNTy(context, width));
    _buildInstrFunction(builder, opcodeTrunc, print, func, exitBlock, disassembler);

    builder.CreateBr(unkownBlock);

    switchInst->addCase(dyn_cast<ConstantInt>(ConstantInt::get(bits->getType(), width)), block);
  }

  builder.SetInsertPoint(unkownBlock);

  if (disassembler) {
    builder.CreateCall(print, builder.CreateGlobalStringPtr("unkown opcode"));
  }
  builder.CreateBr(exitBlock);

  builder.SetInsertPoint(exitBlock);

  if (disassembler) {
    builder.CreateCall(getNamedReg("PC")->getWriteFunction(), oldPC);
  }

  builder.CreateRetVoid();

  return func;
}

void Kernel::_buildInstrFunction(IRBuilder<>& builder, Value* opcode, Value* print, Function* f, BasicBlock* exitBlock, bool disassembler)
{
  LLVMContext& context = llvm::getGlobalContext();

  std::map<uint64_t, std::vector<Instr*>*> opcodeMap;

  for (auto m = _instrs.begin(), e = _instrs.end(); m != e; ++m) {
    if (opcode->getType() != Type::getIntNTy(context, (*m)->getInstrBits()))
      continue;

    auto vec = opcodeMap[(*m)->getOpcodeMask()];

    if (!vec) {
      vec = new std::vector<Instr*>;
      opcodeMap[(*m)->getOpcodeMask()] = vec;
    }

    vec->push_back(*m);
  }

  BasicBlock* block;

  while (!opcodeMap.empty()) {
    auto vecIter = opcodeMap.begin();

    // Get the lagest opcode mask
    for (auto v = opcodeMap.begin(), e = opcodeMap.end(); v != e; ++v) {
      if (v->second->size() > vecIter->second->size()) {
        vecIter = v;
      }
    }

    auto vec = vecIter->second;
    opcodeMap.erase(vecIter);

    block = BasicBlock::Create(context, "", f);

    Value* masked = builder.CreateAnd(opcode, vecIter->first);
    SwitchInst* switchInstr = builder.CreateSwitch(masked, block, vec->size());

    for (auto i = vec->begin(), e = vec->end(); i != e; i++) {
      ConstantInt* iOpcode = dyn_cast<ConstantInt>(ConstantInt::get(opcode->getType(), (*i)->getOpcode()));
      BasicBlock* callBlock = BasicBlock::Create(context, "", f);

      builder.SetInsertPoint(callBlock);

      if (disassembler) {
        Value* f = _cpuModule->getOrInsertFunction((*i)->getDisassembleFunc()->getName(), (*i)->getDisassembleFunc()->getFunctionType());
        builder.CreateCall2(f, print, opcode);
      }
      else {
        Value* f = _cpuModule->getOrInsertFunction((*i)->getCallFunc()->getName(), (*i)->getCallFunc()->getFunctionType());
        builder.CreateCall(f, opcode);
      }
      builder.CreateBr(exitBlock);

      switchInstr->addCase(iOpcode, callBlock);
    }

    builder.SetInsertPoint(block);
  }
}

bool Kernel::_buildInstrExecuter()
{
  Kernel::_buildInstrFunction("kernel.execute", false);

  return true;
}

bool Kernel::_buildInstrDisassembler()
{
  Kernel::_buildInstrFunction("kernel.disassemble", true);

  return true;
}

bool Kernel::_buildCallbackFunction(Twine name, Twine internal_name, vector<Type*>types)
{
  LLVMContext & context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  vector<Type*> callbackTypes(types);
  Type* returnedType = builder.getVoidTy();
  bool hasNonVoidReturn = false;

  for (auto t = types.begin(), e = types.end(); t != e; ++t) {
    if ((*t)->isPointerTy()) {
      returnedType = (*t)->getPointerElementType();
      types.erase(t);
      hasNonVoidReturn = true;
      break;
    }
  }

  FunctionType *funcType =
    FunctionType::get(returnedType, types, false);
  Function *func =
    Function::Create(funcType, Function::ExternalLinkage, internal_name, _m);
  BasicBlock* mainBlock = BasicBlock::Create(context, "", func);
  builder.SetInsertPoint(mainBlock);

  callbackTypes.insert(callbackTypes.begin(), builder.getInt8Ty()->getPointerTo());
  Value* callbackFunc = _m->getOrInsertFunction(name.str(), FunctionType::get(builder.getInt1Ty(), callbackTypes, false));

  vector<Value*> args;
  Value* ret;

  if (hasNonVoidReturn)
    ret = builder.CreateAlloca(returnedType);

  args.push_back(_m->getOrInsertGlobal("callback.context", builder.getInt8Ty()));
  auto a = func->arg_begin();
  for (auto t = ++callbackTypes.begin(), e = callbackTypes.end(); t != e; ++a, ++t) {
    if ((*t)->isPointerTy()) {
      args.push_back(ret);
    }
    else
      args.push_back(a);
  }

  builder.CreateCall(callbackFunc, args);
  if (hasNonVoidReturn)
    builder.CreateRet(builder.CreateLoad(ret));
  else
    builder.CreateRetVoid();
}

bool Kernel::_buildFetchFunction(llvm::Type* type)
{
  vector<Type*>args;
  LLVMContext & context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  args.push_back(builder.getInt32Ty());
  args.push_back(type->getPointerTo());

  return _buildCallbackFunction("callback.fetch" + Twine(type->getIntegerBitWidth()), "cpu.fetch.i" + Twine(type->getIntegerBitWidth()), args);
}

bool Kernel::_buildWriteFunction(llvm::Type* type)
{
  LLVMContext & context = llvm::getGlobalContext();
  IRBuilder<> builder(context);
  vector<Type*>args;

  args.push_back(builder.getInt32Ty());
  args.push_back(type);

  return _buildCallbackFunction("callback.write" + Twine(type->getIntegerBitWidth()), "cpu.write.i" + Twine(type->getIntegerBitWidth()), args);
}

bool Kernel::_link()
{
  Linker* linker = new Linker(_m);

  {
    string what;

    if (linker->linkInModule(_cpuModule, &what)) {
      cerr << "Err" << what;
      return false;
    }
  }

  for (auto m = _instrModules.begin(), e = _instrModules.end(); m != e; ++m) {
    string what;

    if (linker->linkInModule(*m, &what)) {
      cerr << "Err" << what;
      return false;
    }
  }

  return true;
}

Reg* Kernel::getNamedReg(std::string name)
{
  return _regs[name];
}

Flag* Kernel::getNamedFlag(std::string name)
{
  return _flags[name];
}

bool Kernel::writeTo(raw_ostream& stream, bool assembly)
{
  if (assembly) {
    _m->print(stream, 0);
  }
  else {
    WriteBitcodeToFile(_m, stream);
  }

  return true;
}
