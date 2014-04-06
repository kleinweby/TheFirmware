#include "Instr.h"
#include "Kernel.h"
#include "Reg.h"
#include "Flag.h"
#include "InstrOperand.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Casting.h"

#include <iostream>

using namespace std;
using namespace llvm;

vector<Instr*> Instr::parseModule(Kernel* k, llvm::Module* m)
{
  vector<Instr*> instrs;

  for (llvm::Module::iterator f = m->begin(), e = m->end(); f != e; ++f) {
    llvm::NamedMDNode* _metadata = m->getNamedMetadata(Twine(f->getName(), ".instr"));

    if (!_metadata)
      continue;

    if (_metadata->getNumOperands() < 1)
      continue;

    for (int i = 0; i < _metadata->getNumOperands(); i++) {
      MDNode* metadata = _metadata->getOperand(i);

      if (metadata->getNumOperands() < 4)
        continue;

      instrs.push_back(new Instr(k, f, metadata, m));
    }

    m->eraseNamedMetadata(_metadata);
  }

  return instrs;
}

Instr::Instr(Kernel* k, llvm::Function* f, llvm::MDNode* metadata, llvm::Module* m)
  : _f(f), _m(m)
{
  uint32_t op = 0;

  if (MDString* string = dyn_cast_or_null<MDString>(metadata->getOperand(op++))) {
    _mnemonic = string->getString();
  }
  else {
    cerr << "Expected first metadata to be a string" << endl;
  }

  if (ConstantInt* opcodeMask = dyn_cast_or_null<ConstantInt>(metadata->getOperand(op++))) {
    _opcodeMask = opcodeMask->getSExtValue();
    _instrBits = opcodeMask->getBitWidth();
  }
  else {
    cerr << "Opcode Mask not a const int!" << endl;
  }

  if (ConstantInt* opcode = dyn_cast_or_null<ConstantInt>(metadata->getOperand(op++))) {
    _opcode = opcode->getSExtValue();

    if (opcode->getBitWidth() != _instrBits) {
      cerr << "Non matching bit widths" << endl;
    }
  }
  else {
    cerr << "Opcode not a const int!" << endl;
  }

  Twine err;
  if (!InstrOperand::parseOperands(k, _f, metadata, _operands, op, err)) {
    cerr << err.str();
  }
  _buildCallWrapper();
  _buildDisassembleFunction();
  _f->setVisibility(GlobalValue::VisibilityTypes::HiddenVisibility);
  _f->setLinkage(GlobalValue::LinkageTypes::PrivateLinkage);
}

void Instr::_buildCallWrapper()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  vector<llvm::Type *> args;
  args.push_back(Type::getIntNTy(context, _instrBits));
  llvm::ArrayRef<llvm::Type*>  argsRef(args);

  FunctionType *funcType = FunctionType::get(builder.getVoidTy(), args, false);
  Function *mainFunc =
    Function::Create(funcType, Function::ExternalLinkage, Twine(_f->getName(), ".call"), _m);
  mainFunc->setVisibility(Function::ProtectedVisibility);
  BasicBlock *entry = BasicBlock::Create(context, "", mainFunc);
  builder.SetInsertPoint(entry);

  Function::arg_iterator args2 = mainFunc->arg_begin();
  Value* opcode = args2++;
  opcode->setName("opcode");

  vector<Value*> implArgs;
  map<InstrOperand*, Value*> outMap;

  int i = 0;
  for (auto o = _operands.begin(), e = _operands.end(); o != e; ++o, ++i) {
    Value* arg = (*o)->callArgument(builder, opcode);
    Value* val = arg;

    if ((*o)->isOutOperand()) {
      val = builder.CreateAlloca(arg->getType());
      builder.CreateStore(arg, val);
      outMap[*o] = val;
    }

    if (val->getType() != _f->getFunctionType()->getParamType(i)) {
      cerr << _f->getName().str() << " expects ";
      _f->getFunctionType()->getParamType(i)->dump();
      cerr << " but metadata suggests ";
      val->getType()->dump();
      cerr << endl;
    }

    implArgs.push_back(val);
  }

  builder.CreateCall(_f, implArgs);

  for (auto _pair = outMap.begin(), e = outMap.end(); _pair != e; ++_pair) {
    InstrOperand* op = _pair->first;
    Value* val = _pair->second;

    op->finishCallArgument(builder, opcode, builder.CreateLoad(val));
  }

  builder.CreateRetVoid();

  _call = mainFunc;
}

void Instr::_buildDisassembleFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  FunctionType* printFunctionType;

  {
    vector<llvm::Type *> args;
    args.push_back(builder.getInt8Ty()->getPointerTo());

    printFunctionType = FunctionType::get(builder.getVoidTy(), args, true);
  }

  {
    vector<llvm::Type *> args;
    args.push_back(printFunctionType->getPointerTo());
    args.push_back(Type::getIntNTy(context, _instrBits));

    FunctionType *funcType = FunctionType::get(builder.getVoidTy(), args, false);
    _disassemble =
      Function::Create(funcType, Function::ExternalLinkage, Twine(_f->getName(), ".disassemble"), _m);
    _disassemble->setVisibility(Function::ProtectedVisibility);
    BasicBlock *entry = BasicBlock::Create(context, "", _disassemble);
    builder.SetInsertPoint(entry);
  }

  auto funcArgs = _disassemble->arg_begin();
  Value* print = funcArgs++;
  print->setName("print");

  Value* opcode = funcArgs++;
  opcode->setName("opcode");

  Function* formatFunc = _m->getFunction(_f->getName().str() + ".disassemble.format");

  string argsFormat;
  Value* format = NULL;
  vector<Value*> args;
  map<string, Value*> annotations;
  InstrOperand* _lastOp = NULL;

  if (formatFunc) {
    for (auto o = _operands.begin(); o != _operands.end(); o++) {
      args.push_back((*o)->dissasembleArg(builder, opcode));
    }

    format = builder.CreateCall(formatFunc, args);

    StructType* type = dyn_cast<StructType>(format->getType());

    args.erase(args.begin(), args.end());
    for (int i = 0; i < type->getNumElements(); ++i) {
      args.push_back(builder.CreateExtractValue(format, i));
    }

    builder.CreateCall(print, args);
  }
  else {
    for (auto o = _operands.begin(); o != _operands.end(); o++) {
      string str;

      if (_lastOp != NULL && _lastOp->isReverseOperandOf(*o))
        continue;

      _lastOp = *o;

      str = (*o)->dissasembleFormat(builder, opcode, args);

      if (str.length() > 0) {
        if (!argsFormat.empty()) {
          argsFormat += ", ";
        }
        argsFormat += str;
      }
    }

    format = builder.CreateGlobalStringPtr(_mnemonic.str() + " " + argsFormat);

    args.insert(args.begin(), format);
    builder.CreateCall(print, args);
  }

  builder.CreateRetVoid();
}
