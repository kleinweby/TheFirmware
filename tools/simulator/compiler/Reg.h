#pragma once

#include "llvm/IR/Module.h"
#include <map>

class Reg {
protected:
  const unsigned _num;
  std::string _name;
  llvm::Module* _m;
  llvm::Function* _readFunction;
  llvm::Function* _writeFunction;

  virtual void _buildReadFunction();
  virtual void _buildWriteFunction();

  Reg(llvm::Module* m);
  Reg(llvm::Module* m, unsigned num, llvm::MDNode* def);

public:
  static std::map<std::string, Reg*> parseModule(llvm::Module* m);

  virtual bool isRealReg() {
    return true;
  }

  std::string getName() {
    return _name;
  }

  unsigned getNum() {
    return _num;
  }

  llvm::Function* getReadFunction() {
    return _readFunction;
  }

  llvm::Function* getWriteFunction() {
    return _writeFunction;
  }
};
