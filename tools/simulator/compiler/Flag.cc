#include "Flag.h"
#include "Reg.h"
#include "Kernel.h"

#include <iostream>

#include "llvm/IR/IRBuilder.h"

using namespace llvm;
using namespace std;

map<string, Flag*> Flag::parseModule(Kernel* k, llvm::Module* m)
{
  NamedMDNode* _metadata = m->getNamedMetadata("flags");
  MDNode* metadata;
  map<string, Flag*> flags;

  if (!_metadata || _metadata->getNumOperands() != 1) {
    return flags;
  }

  metadata = _metadata->getOperand(0);

  for (unsigned i = 0, e = metadata->getNumOperands(); i < e; i++) {
    MDNode* def = dyn_cast_or_null<MDNode>(metadata->getOperand(i));
    Flag* flag;

    if (!def)
      continue;

    flag = new Flag(k, m, def);

    flags[flag->_name] = flag;
  }

  m->eraseNamedMetadata(_metadata);

  return flags;
}

Flag::Flag(Kernel* k, llvm::Module* m, llvm::MDNode* def)
  : _m(m)
{
  if (def->getNumOperands() != 3) {
    cerr << "flag def requires 2 parameter" << endl;
    return;
  }

  if (MDString* name = dyn_cast_or_null<MDString>(def->getOperand(0))) {
    _name = name->getString();
  }
  else {
    cerr << "flag def requires string parameter" << endl;
    return;
  }

  if (MDString* regName = dyn_cast_or_null<MDString>(def->getOperand(1))) {
    _reg = k->getNamedReg(regName->getString());

    if (!_reg) {
      cerr << "flag def references unkown reg" << regName->getString().str() << endl;
      exit(1);
    }
  }
  else {
    cerr << "flag def requires string parameter" << endl;
    return;
  }

  if (ConstantInt* bit = dyn_cast_or_null<ConstantInt>(def->getOperand(2))) {
    _bit = bit;
  }
  else {
    cerr << "flag def requires int parameter" << endl;
    return;
  }

  _buildReadFunction();
  _buildWriteFunction();
}

void Flag::_buildReadFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  Function* func = _reg->getReadFunction();

  FunctionType *funcType = FunctionType::get(builder.getInt1Ty(), false);
  _readFunction =
    Function::Create(funcType, Function::ExternalLinkage, "read_flag_named_" + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _readFunction);
  builder.SetInsertPoint(entry);

  Value* val = builder.CreateCall(func);

  val = builder.CreateLShr(val, builder.CreateZExtOrTrunc(_bit, val->getType()));
  val = builder.CreateTrunc(val, builder.getInt1Ty());

  builder.CreateRet(val);
}

void Flag::_buildWriteFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  vector<Type*> args;
  args.push_back(builder.getInt1Ty());
  FunctionType *funcType = FunctionType::get(builder.getVoidTy(), args, false);
  _writeFunction =
    Function::Create(funcType, Function::ExternalLinkage, "write_flag_named_" + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _writeFunction);
  builder.SetInsertPoint(entry);

  Function::arg_iterator args2 = _writeFunction->arg_begin();
  Value* value = args2++;

  // Reg old value
  Value* reg = builder.CreateCall(_reg->getReadFunction());
  Value* mask = builder.CreateNot(builder.CreateShl(ConstantInt::get(reg->getType(), 1), builder.CreateZExt(_bit, reg->getType())));
  reg = builder.CreateAnd(reg, mask);

  value = builder.CreateZExt(value, reg->getType());
  value = builder.CreateShl(value, builder.CreateZExt(_bit, value->getType()));

  reg = builder.CreateOr(reg, value);

  builder.CreateCall(_reg->getWriteFunction(), reg);

  builder.CreateRetVoid();
}
