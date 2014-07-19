#!/usr/bin/python

import sys
import os.path
import math
from collections import defaultdict

from elftools.elf.elffile import ELFFile

class Size(object):

  def __init__(self, filename, size):
    self.filename = filename
    self.size = size
    self.category = os.path.splitext(os.path.basename(filename))[0]

    if filename.startswith('core'):
      self.kind = 'core'
      self.name = 'core'
    elif filename.startswith('arch'):
      self.kind = 'arch'
      self.name = os.path.dirname(os.path.relpath(filename, 'arch'))
    elif self.category.endswith('_test'):
      self.kind = 'tests'
      self.category = self.category[:-len('_test')]
      self.name = 'tests'
    elif self.category.endswith('_tests'):
      self.kind = 'tests'
      self.category = self.category[:-len('_tests')]
      self.name = 'tests'
    else:
      self.kind = 'unkown'
      self.name = filename

class FirmwareSizer(object):

  def __init__(self, filename):
    self.categories = defaultdict(list)
    self.max_name_length = 0
    self.max_cat_length = 0
    self.max_size = 0
    self.parse(filename)

  def dump(self):
    kind_sizes = defaultdict(int)

    for cat in sorted(self.categories.keys()):
      size_list = self.categories[cat]
      total = sum(s.size for s in size_list)
      self.dump_line(0, cat, total)

      if len(size_list) == 1 and size_list[0].kind == 'core':
        kind_sizes[size_list[0].kind] += size_list[0].size
        continue

      for s in sorted(size_list, key=lambda s: s.kind):
        self.dump_line(1, s.name, s.size)
        kind_sizes[s.kind] += s.size

    self.dump_separator()

    total = sum(kind_sizes.values())
    for name, size in kind_sizes.iteritems():
      self.dump_line(0, name, size, 100 * size/float(total))
    self.dump_line(0, 'total', total)

  def dump_separator(self):
    width = max(self.max_cat_length, self.max_name_length + len(' - ')) + len(': ') + int(math.ceil(math.log10(self.max_size)))+ len(' (99.9%)')
    print(width * '-')

  def dump_line(self, ident, name, size, percent = -1.0):
    width = max(self.max_cat_length, self.max_name_length + len(' - ')) + len(': ')
    line = ''
    if ident > 0:
      line += ' ' * ident + '- '
    line += name + ": "
    line = line.ljust(width)
    line += ("%d" % size).rjust(int(math.ceil(math.log10(self.max_size))))
    if percent >= 0.0:
      line += " (%02.1f%%)" % percent
    print(line)

  def parse(self, filename):
    with open(filename, 'rb') as f:
      elffile = ELFFile(f)

      if not elffile.has_dwarf_info():
        return

      dwarfinfo = elffile.get_dwarf_info()
      infos = {}
      for CU in dwarfinfo.iter_CUs():
        infos[CU.get_top_DIE().get_full_path()] = CU['unit_length']

      common_prefix = os.path.commonprefix(infos.keys())

      for path, size in infos.iteritems():
        s = Size(os.path.relpath(path, common_prefix), size)
        self.categories[s.category].append(s)

        self.max_name_length = max(self.max_name_length, len(s.name))
        self.max_cat_length = max(self.max_cat_length, len(s.category))
        self.max_size += s.size

if __name__ == '__main__':
  FirmwareSizer(sys.argv[1]).dump()
