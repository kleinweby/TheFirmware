#!/usr/bin/python

from pyparsing import Word, alphas, alphanums, delimitedList, LineEnd, LineStart, ZeroOrMore, Literal, Combine, Group
import sys
import click
import os.path

path = Literal('/').suppress() + delimitedList(Word(alphas, alphanums + "-_"), "/", combine=True)('path')

func_obj = Literal('func')('type') + Word(alphas, alphanums + "_")('func_name')
obj = func_obj
defi = Group(path + ':' + obj)

fs_file = ZeroOrMore(defi)('defs')

class FSObject(object):
	def entry_func(self):
		return 'UNKOWN'

	def entry_extra_arguments(self):
		return []

	def emit(self, f):
		pass

	def emit_declarations(self, root, f):
		pass

class FSDir(FSObject):
	
	def __init__(self, name):
		super(FSDir, self).__init__()
		self.name = name
		self.entries = {}

	def emit(self, f):
		for entry in self.entries.itervalues():
			entry.emit(f)

		f.write("STATICFS_DIR(%s,\n" % self.name);

		for name, entry in self.entries.iteritems():
			args = ["\"%s\"" % name] + entry.entry_extra_arguments()

			f.write("    %s(%s),\n" % (entry.entry_func(), ', '.join(args)))

		f.write("    STATICFS_DIR_ENTRY_LAST,\n");
		f.write(");\n\n")

	def add_entry(self, name, obj):
		self.entries[name] = obj

	def get_entry(self, name):
		if name in self.entries:
			return self.entries[name]
		else:
			return None

	def entry_func(self):
		return 'STATICFS_DIR_ENTRY'

	def entry_extra_arguments(self):
		return [self.name]

	def emit_declarations(self, root, f):
		for entry in self.entries.itervalues():
			entry.emit_declarations(root, f)

class FSExecutable(FSObject):
	def __init__(self, func_name):
		super(FSExecutable, self).__init__()

		self.func_name = func_name

	def entry_func(self):
		return 'STATICFS_DIR_ENTRY_CALLABLE'

	def entry_extra_arguments(self):
		return [self.func_name]

	def emit_declarations(self, root, f):
		root.emit_func_delc(self.func_name, 'int', 'int argc, const char** argv')

class FSRoot(FSDir):

	def __init__(self):
		super(FSRoot, self).__init__('root')

		self.func_declarations = []
		self.f = None

	def emit_func_delc(self, name, ret, args):
		if name in self.func_declarations:
			return

		self.f.write("extern %s %s(%s);\n\n" % (ret, name, args))
		self.func_declarations.append(name)

	def emit(self, f):
		self.f = f
		f.write('//Generated. DO NOT EDIT\n')
		f.write('#include <vfs.h>\n\n')
		self.emit_declarations(self, f)
		super(FSRoot, self).emit(f)
		f.write('vnode_t staticfs_root = (vnode_t)&root_staticnode;\n')

root = FSRoot()

@click.command()
@click.option('--output', '-o', default='-', type=click.File('wb'))
@click.argument('src', type=click.File(), nargs=-1)
def gen_fs(output, src):
	for src in src:
		content = fs_file.parseString(src.read(), True)
	
		if 'defs' in content:
			for test in content['defs']:
				path = test['path']
				t = test['type']
				
				parent = root

				components = path.split('/')

				for i, name in enumerate(components[:-1]):
					dir = parent.get_entry(name)
					if dir is None:
						dir = FSDir('_'.join(components[0:i+1]))
						parent.add_entry(name, dir)

					parent = dir

				if t == 'func':
					parent.add_entry(components[-1], FSExecutable(test['func_name']))

	root.emit(output)	

gen_fs()
