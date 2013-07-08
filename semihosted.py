import gdb

class StringBreakpoint(gdb.Breakpoint):
	def __init__(self, spec, var):
		super(StringBreakpoint, self).__init__(spec, gdb.BP_BREAKPOINT, 0, True)

		self.var = var

	def stop(self):
		s = gdb.parse_and_eval(self.var)

		sys.stdout.write(s.string())

		return False

class CharBreakpoint(gdb.Breakpoint):
	def __init__(self, spec, var):
		super(CharBreakpoint, self).__init__(spec, gdb.BP_BREAKPOINT, 0, True)

		self.var = var

	def stop(self):
		s = gdb.parse_and_eval(self.var)

		sys.stdout.write(unichr(s))

		return False

StringBreakpoint("TheFirmware::Log::printString", "string")
CharBreakpoint("TheFirmware::Log::printChar", "c")