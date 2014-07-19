LLVM_CONFIG=/usr/local/Cellar/llvm/3.4/bin/llvm-config
LLVM_CXXFLAGS=$(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags --libs object)

CXXFLAGS=$(LLVM_CXXFLAGS) -g -ggdb -O0 -std=c++11
LDFLAGS=$(LLVM_LDFLAGS) -g -ggdb -std=c++11
SRC=main.cc
OBJS=$(SRC:.cc=.o)
EXEC=gen-fs

$(EXEC): $(OBJS)
	clang++ -o $@ $^ $(LDFLAGS)

.cc.o:
	clang++ -c -MMD -MF $<.d -o $@ $< $(CXXFLAGS)

clean:
	rm -rf $(EXEC) $(OBJS) $(SRC:.cc=.cc.d)
