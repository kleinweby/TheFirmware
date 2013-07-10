import gdb

class StringBreakpoint(gdb.Breakpoint):
	def __init__(self, spec, var):
		super(StringBreakpoint, self).__init__(spec, gdb.BP_BREAKPOINT, 0, True)

		self.var = var

	def stop(self):
		s = gdb.parse_and_eval(self.var)

		sys.stdout.write(s.string())

		return False

StringBreakpoint("TheFirmware::Log::flushString", "string")
