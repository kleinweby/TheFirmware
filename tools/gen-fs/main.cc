
#include <iostream>

#include "gen-core.h"

class TestEntry: public CStruct {

	StringRef _name;

	StringElement _path;
	PtrElement _ptr;
	Element<uint8_t> _flags;

public:
	TestEntry(StringRef name, ObjectFile* obj) : _name(name), _path(obj), _ptr(obj), _flags(obj)
	{
		setElements({&_path, &_ptr, &_flags});
	}

	string str() {
		return _name.str() + " path=" + _path.getString().str() + " symbol=" + _ptr.getSymbolName().str() + " flags=" + Twine((uint32_t)_flags.getData()).str();
	}
};

int main(int argc, char** argv)
{
	CStructExtractor<TestEntry> extractor("/Users/christian/Documents/Projects/TheFirmware/.build/core/bootstrap.c.o", "fs");

	for (auto entry: extractor.getEntries()) {
		cout << "Got: " << entry->str() << endl;
	}

	return 0;
}
