#pragma once

#include "llvm/IR/Module.h"
#include <vector>

class InstrOperand;
class Kernel;

class Instr {
  llvm::Function* _f;
  llvm::Function* _call;
  llvm::Function* _disassemble;
  llvm::Module* _m;

  llvm::StringRef _mnemonic;
  uint64_t _instrBits;
  uint64_t _opcodeMask;
  uint64_t _opcode;
  std::vector<InstrOperand*> _operands;

  Instr(Kernel* k, llvm::Function* f, llvm::MDNode* metadata, llvm::Module* m);

  void _buildCallWrapper();
  void _buildDisassembleFunction();

public:
  static std::vector<Instr*> parseModule(Kernel* k, llvm::Module* m);

  uint64_t getOpcodeMask() {
    return _opcodeMask;
  }

  uint64_t getOpcode() {
    return _opcode;
  }

  uint64_t getInstrBits() {
    return _instrBits;
  }

  llvm::Function* getCallFunc() {
    return _call;
  }

  llvm::Function* getDisassembleFunc() {
    return _disassemble;
  }
};
