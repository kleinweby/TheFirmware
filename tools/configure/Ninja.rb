class NinjaBase
	attr_accessor :parent
	attr_accessor :vars

	def initialize(parent)
		self.vars = {}
		self.parent = parent
	end

	def var(name, value = nil, override = false)
		if value.nil?
			self.vars[name.to_sym]
		else
			value = [ value ] unless value.is_a? Enumerable

			if !override and self.vars[name.to_sym]
				self.vars[name.to_sym] += value
			else
				self.vars[name.to_sym] = value
			end
		end
	end

	def to_ninja(indentation = 0)
		str = ""

		vars = self.vars.map do |k,v|
			v.reverse! if k.to_s.end_with? '_paths'

			v = v.map do |v|
				v.to_s
			end

			refs = v.map do |v|
				v[1..v.length].to_sym if v.start_with? "$"
			end.compact

			v = v.join(" ")

			if declares_parent_var k
				v = "$#{k} #{v}"
			end

			[k, {
				:values => v,
				:refs => refs
			}]
		end

		changed = true
		while changed
			changed = false
			seen_keys = []

			vars.each_index do |idx|
				k = vars[idx][0]
				refs = vars[idx][1][:refs]

				missing_refs = refs - seen_keys

				missing_refs.each do |missing|
					new_idx = vars.find_index do |var|
						var[0] == missing
					end

					unless new_idx.nil?
						t = vars[new_idx]
						vars.delete_at new_idx
						vars.insert idx, t
						seen_keys << missing
						changed = true
					end
				end

				seen_keys << k
			end
		end

		vars.each do |t|
			k = t[0]
			v = t[1]

			str += self.format_line "#{k} = #{v[:values]}", indentation
		end

		str
	end 

	def format_line(line, indent = 0)
		prefix = '  ' * indent
		is_breaking = false

		output = ""
		width = 80

		while indent + line.length > width
			max_idx = width - prefix.length - 2 # 2 because we need the space and the $ at the end

			while true
				idx = line.rindex(' ', max_idx)

				if idx.nil?

					max_idx += 1
          break if max_idx > line.length
				elsif self.is_escaped_at? line, idx

					max_idx = idx - 1
					if max_idx <= 0
						break
					end
				else
					output += prefix + line[0..idx] + "$\n"
					line = line[idx + 1..line.length]

					prefix += '    ' unless is_breaking
					is_breaking = true

					break
				end
			end

			break if max_idx > line.length or max_idx <= 0
		end

		output + prefix + line + "\n"
	end

	@private
	def declares_parent_var(name)
		parent = self.parent

		while not parent.nil?
			return true unless parent.var(name).nil?

			parent = parent.parent
		end

		false
	end

	def is_escaped_at?(str, index)
		index -= 1
		count = 0

		while index >= 0
			if str[index] == '$'
				count += 1
			else
				break
			end

			inddex -= 1
		end

		count % 2 != 0
	end
end

class Ninja < NinjaBase
	class Rule < NinjaBase

		def initialize(parent, name)
			super(parent)

			@name = name
		end

		def to_ninja(indentation = 0)
			str = self.format_line "rule #{@name}", indentation

			str + super(indentation + 1) + "\n"
		end
	end

	class Build < NinjaBase

		def initialize(parent, name, rule, inputs, implict=nil)
			super(parent)

			@name = name
			@rule = rule
			@inputs = inputs
			@implict = implict
		end

		def to_ninja(indentation = 0)
			extra_deps = ""

			extra_deps += " | " + prepare(@implict) unless @implict.nil?

			str = self.format_line "build #{prepare(@name)}: #{@rule} #{prepare(@inputs)}#{extra_deps}", indentation

			str + super(indentation + 1) + "\n"
		end

		def prepare(arg)
			arg = arg.eval if arg.respond_to? :eval
			arg = arg.join(" ") if arg.is_a? Enumerable

			arg
		end
	end

  attr_accessor :defs

	def initialize(parent = nil)
		super(parent)

		self.defs = []
	end

	def build(name, rule, inputs=nil, implict=nil, &block)
		b = Build.new(self, name, rule, inputs, implict)

		block.arity < 1 ? b.instance_eval(&block) : block.call(b) if block_given?

		self.defs << b
	end

	def rule(name, &block)
		r = Rule.new(self, name)

		block.arity < 1 ? r.instance_eval(&block) : block.call(r) if block_given?

		self.defs << r
	end

	def to_ninja(indentation = 0)
		defs = self.defs.map do |d|
			if d.is_a? Ninja
				d.vars.each do |k,v|
					var k, v
				end

				d.defs
			else
				d
			end
		end.flatten

		str = super(indentation) + "\n"

		defs.each do |d|
			str += d.to_ninja
		end

		str
	end
end
