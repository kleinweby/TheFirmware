import gdb

class PrintfBreakpoint(gdb.Breakpoint):
	def __init__(self, spec, formatVariable):
		super(PrintfBreakpoint, self).__init__(spec, gdb.BP_BREAKPOINT, 0, True)

		self.formatVariable = formatVariable

	def stop(self):
		format = gdb.parse_and_eval(self.formatVariable)

		print(self.formatString(format))

		return False

	def formatString(self, formatValue):
		format = formatValue.string()
		address = formatValue.address + 1

		s = ""

		iterator = iter(format)

		try:
			for c in iterator:
				if c == '%':
					c = iterator.next()

					if c == '%':
						string += '%'
					elif c == 's':
						s += str(address.dereference())
						address += 1
					elif c == 'i':
						s += str(int(address.cast(gdb.lookup_type("int").pointer()).dereference()))
						address += 1
				else:
					s += c
		except StopIteration, e:
			print ("prematur end")

		return s

PrintfBreakpoint("printf", "format")