#pragma once

#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include <map>

class Reg;
class Kernel;

class Flag {
protected:
  Reg* _reg;
  llvm::ConstantInt* _bit;
  std::string _name;
  llvm::Module* _m;
  llvm::Function* _readFunction;
  llvm::Function* _writeFunction;

  virtual void _buildReadFunction();
  virtual void _buildWriteFunction();

  Flag(Kernel* k, llvm::Module* m, llvm::MDNode* def);

public:
  static std::map<std::string, Flag*> parseModule(Kernel* k, llvm::Module* m);

  std::string getName() {
    return _name;
  }

  llvm::Function* getReadFunction() {
    return _readFunction;
  }

  llvm::Function* getWriteFunction() {
    return _writeFunction;
  }
};
