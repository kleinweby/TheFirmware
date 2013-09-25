require_relative 'DefaultConfig'
require_relative 'VersionSupport'

RakeFileUtils.verbose_flag = false # if RakeFileUtils.verbose_flag == RakeFileUtils::DEFAULT

# Rule for c -> o (with deps)
rule '.o' => [
	proc { |tn| tn.sub(/\.o$/, '.c').sub(/^#{OBJ_DIR}\//, '') }
] do |t|
	puts " [CC]   #{t.source}"
	FileUtils.mkdir_p(File.dirname(t.name))
	sh "#{CC} -MM -MT #{t.name} -c -o #{t.name.ext('depend')} #{t.source}  -std=c1x #{CFLAGS.join(' ')} #{DEFINES.join(' ')}"
	sh "#{CC} -c -o #{t.name} #{t.source} #{CFLAGS.join(' ')}  -std=c1x #{DEFINES.join(' ')}"
end

# Rule for c -> E
rule '.E' => [ '.c' ] do |t|
	puts " [CPP] #{t.source}"
	sh "#{CC} -E -o #{t.name} #{t.source} #{CFLAGS.join(' ')} #{DEFINES.join(' ')}"
end

# Rule for cc -> o (with deps)
rule '.o' => [
	proc { |tn| tn.sub(/\.o$/, '.cc').sub(/^#{OBJ_DIR}\//, '') }
] do |t|
	puts " [CXX]  #{t.source}"
	FileUtils.mkdir_p(File.dirname(t.name))
	sh "#{CC} -MM -MT #{t.name} -c -o #{t.name.ext('depend')} #{t.source} -std=c++11 -Wno-c++98-compat-pedantic -fno-exceptions -fno-rtti #{CXXFLAGS.join(' ')} #{DEFINES.join(' ')}"
	sh "#{CC} -c -o #{t.name} #{t.source} -std=c++11 -Wno-c++98-compat-pedantic -fno-exceptions -fno-rtti #{CXXFLAGS.join(' ')} #{DEFINES.join(' ')}"
end

# Rule for cc -> E
rule '.E' => [ '.cc' ] do |t|
	puts " [CPP] #{t.source}"
	sh "#{CC} -E -o #{t.name} #{t.source} #{CFLAGS.join(' ')} #{DEFINES.join(' ')}"
end

# Rule for nasm -> E
rule '.o' => [
	proc { |tn| tn.sub(/\.o$/, '.nasm').sub(/^#{OBJ_DIR}\//, '') }
] do |t|
	puts " [NSAM] #{t.source}"
	FileUtils.mkdir_p(File.dirname(t.name))
	sh "#{NASM} -f elf -g -o #{t.name} #{t.source} #{DEFINES.join(' ')}"
end
