import gdb.printing
import itertools

class Timeout(object):

	def __init__(self, timeout):
		self.timeout = timeout

	def to_string(self):
		options = []

		if self.timeout['fired'] == True:
			options.append("fired")

		if self.timeout['repeat'] == True:
			options.append("repeat")

		if self.timeout['attached'] == True:
			options.append("attached")

		return "<%s %d/%d %s>" % (self.timeout.type, self.timeout['remainingTime'], self.timeout['timeout'], " ".join(options))

class TimeoutIterator(object):
	
	def __init__(self, timeout):
		self.timeout = timeout

	def __iter__(self):
		return self

	def next(self):
		timeout = self.timeout

		if timeout == 0:
			raise StopIteration

		self.timeout = self.timeout.dereference()['next']

		return timeout.dereference()

class TimerPrinter(object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "Some timer %s" % str(self.val.type)

    def children(self):
    	counter = itertools.imap(lambda i: '[%d]' % i, itertools.count(1))
    	timeouts = TimeoutIterator(self.val['timeouts'])
    	return itertools.izip(counter, timeouts)

    def display_hint (self):
    	return 'array'

def build_pretty_printer():
         pp = gdb.printing.RegexpCollectionPrettyPrinter(
             "TheFirmware")
         pp.add_printer('timer', '^TheFirmware::Time::Timer$', TimerPrinter)
         pp.add_printer('timeout', '^TheFirmware::Time::Timeout$', Timeout)
         return pp

gdb.printing.register_pretty_printer(
         gdb.current_objfile(),
         build_pretty_printer(), replace=True)
