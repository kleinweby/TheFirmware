#include <string>
#include <map>
#include <llvm/Object/ObjectFile.h>

using namespace std;
using namespace llvm;
using namespace llvm::object;

class BaseElement {

public:
	BaseElement()
	{}

	virtual void updateData(const char*& ptr) = 0;
	virtual size_t dataLength() = 0;
	virtual void updateRelocation(RelocationRef reloc) = 0;
};

template<class T> class Element: public BaseElement {
protected:
	T _data;

	ObjectFile* _obj;
public:

	Element(ObjectFile* obj) : _obj(obj)
	{}

	virtual void updateData(const char*& ptr)
	{
		_data = *(T*)ptr;

		ptr += sizeof(T);
	}

	virtual size_t dataLength()
	{
		return sizeof(T);
	}

	T getData() const
	{
		return _data;
	}

	virtual void updateRelocation(RelocationRef reloc)
	{
		cout << "Tried to update relocation on plain value" << endl;
	};
};

class PtrElement : public Element<uint32_t> {
protected:
	SymbolRef _symbol;

	SectionRef getTargetSection()
	{
		section_iterator section = _obj->begin_sections();

		_symbol.getSection(section);

		return *section;
	}

public:
	PtrElement(ObjectFile* obj) : Element(obj)
	{}

	virtual void updateRelocation(RelocationRef reloc)
	{
		_symbol = *reloc.getSymbol();
	};

	StringRef getSymbolName()
	{
		StringRef name;
		_symbol.getName(name);
		return name;
	}
};

class StringElement : public PtrElement {
public:
	StringElement(ObjectFile* obj) : PtrElement(obj)
	{}

	StringRef getString() {
		string str;
		StringRef contents;
		uint64_t addr;

		_symbol.getAddress(addr);

		addr += _data;

		getTargetSection().getContents(contents);

		while(addr < contents.size()) {
			char c = contents[addr];

			if (c == '\0')
				break;

			str += c;
			addr = addr + 1;
		}

		return str;
	}
};

class CStruct {
	vector<BaseElement*> _elements;
	map<uint64_t, BaseElement*> _offsettedElements;
	uint64_t _size;

protected:
	void setElements(vector<BaseElement*> elements)
	{
		_elements = elements;

		uint64_t offset = 0;

		for (BaseElement* e: _elements) {
			_offsettedElements[offset] = e;
			offset += e->dataLength();
		}

		_size = offset;
	}

public:

	void update(SectionRef section)
	{
		bool is_data;

		section.isData(is_data);

		if (is_data) {
			uint64_t size;
			section.getSize(size);

			if (size != _size) {
				cerr << "Expected size " << size << " got " << _size << endl;
				abort();
			}

			StringRef contents;
			section.getContents(contents);

			const char* data = contents.data();

			for (BaseElement* e : _elements) {
				e->updateData(data);
			}
		}
		else {
			llvm::error_code err;
			for (auto rel = section.begin_relocations(), rel_end = section.end_relocations(); rel != rel_end; rel = rel.increment(err)) {
				uint64_t offset;

				rel->getOffset(offset);

				_offsettedElements[offset]->updateRelocation(*rel);
			}
		}
	}
};

template<class T>
class CStructExtractor {
	ObjectFile* _object;
	StringRef _ns;
	map<StringRef, T*> _entries;

	void _extract()
	{
		llvm::error_code err;
		for (auto section = _object->begin_sections(), section_end = _object->end_sections(); section != section_end; section = section.increment(err)) {
			StringRef name;
			uint64_t length;
			section->getName(name);

			if (!name.startswith(_ns) && !name.startswith(".rel" + _ns.str()))
				continue;

			auto name_pair = name.split(_ns);

			T*& entry = _entries[name_pair.second];

			if (!entry) {
				entry = new T(name_pair.second, _object);
			}

			entry->update(*section);
		}
	}

public:
	CStructExtractor(StringRef filename, StringRef __ns)
	{
		auto obj = ObjectFile::createObjectFile(filename);

		if (!obj) {
			cout << "Open failed" << endl;
		}

		string ns = __ns.str();

		if (!__ns.startswith("."))
			ns = "." + ns;
		if (!__ns.endswith("."))
			ns = ns + ".";

		_ns = ".text.gen" + ns;

		_object = obj;
		_extract();
	}

	vector<T*> getEntries()
	{
		vector<T*> entries;

		for (auto e: _entries) {
			entries.push_back(e.second);
		}

		return entries;
	}
};
