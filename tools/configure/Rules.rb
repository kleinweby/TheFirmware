
rule 'cc' do 
	var :depfile, '$out.d'
	var :command, '$cc -MMD -MF $out.d -std=c1x $cflags -c $in -o $out'
	var :description, "CC   $in"
end

register_default_rule '.c', 'cc'

rule 'cxx' do
	var :depfile, '$out.d'
	var :command, '$cxx -MMD -MF $out.d -std=c++11 $cxxflags -c $in -o $out'
	var :description, "CXX  $in"
end

register_default_rule '.cc', 'cxx'

rule 'ld' do
	var :command, '$ld -T$linker_file -o $out $in $ldflags'
	var :description, "LD   $out"
end

rule 'configure' do
	var :command, './configure.rb $configure_args'
	var :generator, '1'
	var :description, 'CONF ./configure.rb $configure_args'
end

rule 'gen' do
	var :command, "./tools/configure/gen.rb $out"
	var :description, 'GEN $out'
end
