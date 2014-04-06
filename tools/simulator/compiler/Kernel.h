
#pragma once

#include <string>
#include <vector>
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/SourceMgr.h"

class Instr;
class Reg;
class Flag;

class Kernel {
  const std::string& _dir;
  std::vector<Instr*> _instrs;
  std::vector<llvm::Module*> _instrModules;
  std::map<std::string, Reg*> _regs;
  std::map<std::string, Flag*> _flags;
  llvm::Module* _cpuModule;
  llvm::Module* _m;

  llvm::Module* parseInstrFile(std::string filename, llvm::SMDiagnostic &Err, llvm::LLVMContext &Context);

  bool _parseCPUDefinition();
  bool _parseRegDefs();
  bool _parseInstructions();
  bool _parseInstructionFile(const std::string& filename);

  void _buildInstrFunction(llvm::IRBuilder<>& builder, llvm::Value* opcode, llvm::Value* print, llvm::Function* f, llvm::BasicBlock* exit, bool disassembler);
  llvm::Function* _buildInstrFunction(const std::string name, bool disassembler);
  bool _buildInstrExecuter();
  bool _buildInstrDisassembler();

  bool _buildCallbackFunction(llvm::Twine name, llvm::Twine internal_name, std::vector<llvm::Type*>types);
  bool _buildFetchFunction(llvm::Type* type);
  bool _buildWriteFunction(llvm::Type* type);

  bool _link();

public:
  Kernel(const std::string& directory);

  bool build();

  Reg* getNamedReg(std::string name);
  Flag* getNamedFlag(std::string name);

  bool writeTo(llvm::raw_ostream& stream, bool assembly);
};
