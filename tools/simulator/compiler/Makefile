LLVM_CONFIG=/usr/local/Cellar/llvm/3.4/bin/llvm-config
LLVM_CXXFLAGS=$(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags --libs)

CXXFLAGS=$(LLVM_CXXFLAGS) -g -ggdb -std=c++11
LDFLAGS=$(LLVM_LDFLAGS) -g -ggdb -std=c++11
SRC=compiler.cc Instr.cc InstrOperand.cc Reg.cc Flag.cc Kernel.cc
OBJS=$(SRC:.cc=.o)

compiler: $(OBJS)
	clang++ -o $@ $^ $(LDFLAGS)

.cc.o:
	clang++ -c -MMD -MF $<.d -o $@ $< $(CXXFLAGS)
	
clean:
	rm -rf compiler $(OBJS) $(SRC:.cc=.cc.d)

-include $(SRC:.cc=.cc.d)
