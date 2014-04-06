#include "Reg.h"

#include <iostream>

#include "llvm/IR/IRBuilder.h"

using namespace llvm;
using namespace std;

class VirtualReg : public Reg {
  Reg* _baseReg;
  ConstantInt* _mask;
  ConstantInt* _shift;

  virtual void _buildReadFunction();
  virtual void _buildWriteFunction();

public:
  VirtualReg(Module* m, MDNode* def, map<string, Reg*> known_regs);

  bool isRealReg() {
    return false;
  }
};

map<string, Reg*> Reg::parseModule(llvm::Module* m)
{
  NamedMDNode* _metadata = m->getNamedMetadata("regs");
  MDNode* metadata;
  map<string, Reg*> regs;

  if (!_metadata || _metadata->getNumOperands() != 1) {
    cerr << "Metadata !regs missing from cpu definition." << endl;
    exit(1);
  }

  metadata = _metadata->getOperand(0);

  for (unsigned i = 0, e = metadata->getNumOperands(); i < e; i++) {
    MDNode* regDef = dyn_cast_or_null<MDNode>(metadata->getOperand(i));
    Reg* reg;

    if (!regDef)
      continue;

    if (regDef->getNumOperands() > 1) {
      reg = new VirtualReg(m, regDef, regs);
    }
    else {
      reg = new Reg(m, i, regDef);
    }

    regs[reg->_name] = reg;
  }

  m->eraseNamedMetadata(_metadata);

  return regs;
}

Reg::Reg(llvm::Module* m)
  : _num(0), _m(m)
{}

Reg::Reg(llvm::Module* m, unsigned num, llvm::MDNode* def)
  : _num(num), _m(m)
{
  if (def->getNumOperands() != 1) {
    cerr << "Normal reg def requires 1 parameter" << endl;
    return;
  }

  if (MDString* name = dyn_cast_or_null<MDString>(def->getOperand(0))) {
    _name = name->getString();
  }
  else {
    cerr << "norml reg def requires string parameter" << endl;
    return;
  }

  _buildReadFunction();
  _buildWriteFunction();
}

void Reg::_buildReadFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  Function* read_reg = _m->getFunction("cpu.reg.read");

  if (!read_reg) {
    cerr << "read_reg is not avaiable" << endl;
    exit(1);
  }

  FunctionType *funcType = FunctionType::get(read_reg->getReturnType(), false);
  _readFunction =
    Function::Create(funcType, Function::ExternalLinkage, "cpu.reg.read." + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _readFunction);
  builder.SetInsertPoint(entry);

  Value* val = builder.CreateCall(read_reg, ConstantInt::get(read_reg->getFunctionType()->getParamType(0), _num));

  builder.CreateRet(val);
}

void Reg::_buildWriteFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  Function* write_reg = _m->getFunction("cpu.reg.write");

  if (!write_reg) {
    cerr << "write_reg is not avaiable" << endl;
    exit(1);
  }

  vector<Type*> args;
  args.push_back(write_reg->getFunctionType()->getParamType(1));
  FunctionType *funcType = FunctionType::get(write_reg->getReturnType(), args, false);
  _writeFunction =
    Function::Create(funcType, Function::ExternalLinkage, "cpu.reg.write." + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _writeFunction);
  builder.SetInsertPoint(entry);

  Function::arg_iterator args2 = _writeFunction->arg_begin();
  Value* value = args2++;

  builder.CreateCall2(write_reg,
    ConstantInt::get(write_reg->getFunctionType()->getParamType(0), _num),
    value);

  builder.CreateRetVoid();
}

VirtualReg::VirtualReg(Module* m, MDNode* def,  map<string, Reg*> known_regs)
  : Reg(m)
{
  if (def->getNumOperands() != 4) {
    cerr << "virtual reg def requires 4 parameter" << endl;
    return;
  }

  if (MDString* name = dyn_cast_or_null<MDString>(def->getOperand(0))) {
    _name = name->getString();
  }
  else {
    cerr << "virtual reg def requires string parameter" << endl;
    return;
  }

  if (MDString* baseRegName = dyn_cast_or_null<MDString>(def->getOperand(1))) {
    Reg* reg = known_regs[baseRegName->getString()];

    if (!reg) {
      cerr << "virtual reg base not found" << endl;
      return;
    }

    _baseReg = reg;
  }
  else {
    cerr << "virtual reg def requires string parameter" << endl;
    return;
  }

  if (ConstantInt* mask = dyn_cast_or_null<ConstantInt>(def->getOperand(2))) {
    _mask = mask;
  }
  else {
    cerr << "virtual reg def requires int parameter" << endl;
    return;
  }

  if (ConstantInt* shift = dyn_cast_or_null<ConstantInt>(def->getOperand(3))) {
    _shift = shift;
  }
  else {
    cerr << "virtual reg def requires int parameter" << endl;
    return;
  }

  _buildReadFunction();
  _buildWriteFunction();
}

void VirtualReg::_buildReadFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  FunctionType *funcType = FunctionType::get(_baseReg->getReadFunction()->getReturnType(), false);
  _readFunction =
    Function::Create(funcType, Function::ExternalLinkage, "read_reg_named_" + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _readFunction);
  builder.SetInsertPoint(entry);

  Value* val = builder.CreateCall(_baseReg->getReadFunction());
  val = builder.CreateAnd(val, builder.CreateBitCast(_mask, val->getType()));
  val = builder.CreateLShr(val, builder.CreateZExtOrTrunc(_shift, val->getType()));

  builder.CreateRet(val);
}

void VirtualReg::_buildWriteFunction()
{
  LLVMContext& context = llvm::getGlobalContext();
  IRBuilder<> builder(context);

  Function* write_reg = _baseReg->getWriteFunction();

  vector<Type*> args;
  args.push_back(write_reg->getFunctionType()->getParamType(0));
  FunctionType *funcType = FunctionType::get(write_reg->getReturnType(), args, false);
  _writeFunction =
    Function::Create(funcType, Function::ExternalLinkage, "write_reg_named_" + _name, _m);
  BasicBlock *entry = BasicBlock::Create(context, "", _writeFunction);
  builder.SetInsertPoint(entry);

  Function::arg_iterator args2 = _writeFunction->arg_begin();
  Value* value = args2++;

  // Reg old value
  Value* reg = builder.CreateCall(_baseReg->getReadFunction());
  reg = builder.CreateAnd(reg, builder.CreateBitCast(builder.CreateNot(_mask), reg->getType()));

  value = builder.CreateShl(value, builder.CreateZExtOrTrunc(_shift, value->getType()));
  value = builder.CreateAnd(value, builder.CreateBitCast(_mask, value->getType()));

  reg = builder.CreateOr(reg, value);

  builder.CreateCall(write_reg, reg);

  builder.CreateRetVoid();
}
