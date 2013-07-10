require_relative './BuildSupport/Common'
require 'rake/clean'
require 'rake/loaders/makefile'
require 'json'
require 'tempfile'

SRC=FileList[]

SRC.include([
  'CMSIS/src/core_cm0.c',
  'CMSIS/src/system_LPC11xx.c',
  'Target/cr_startup_lpc11.c',
  'main.cc',
  'Firmware/Log.cc',
])

FIRMWARE_SRC = FileList[
	'Firmware/IO/OStream.cc',
	'Firmware/IO/GDBSemihostedOStream.cc',
	'Firmware/Runtime.cc',
]

SRC.include(FIRMWARE_SRC)

OBJ = SRC.ext('o').pathmap("#{OBJ_DIR}/%p")
DEPS = OBJ.ext('depend')

LDFLAGS << "-lgcc"
CXXFLAGS << '-funsigned-char' << '-Wno-deprecated-register'

LINKER_SCRIPT = 'Target/link.ld'
GDB_SCRIPT = 'Target/startup.gdb'
EXECUTABLE_NAME = 'firmware'

CLEAN.include(OBJ_DIR)

task :default => [ "#{EXECUTABLE_NAME}.elf" ]

file "#{EXECUTABLE_NAME}.elf" => [ *OBJ, LINKER_SCRIPT ]  do |t|
	puts " [LD] #{t.name}"
	sh "#{LD} -T#{LINKER_SCRIPT} -o #{t.name} #{OBJ.join(' ')} #{LDFLAGS.join(' ')}"
end

task 'jlink' do |t|
	sh "/Applications/JLink/JLinkGDBServer.command -if SWD -device LPC11C24 -vd"
end

task 'debug'  => ["#{EXECUTABLE_NAME}.elf", GDB_SCRIPT] do |t|
	trap('INT') {}
	sh "#{GDB} --init-eval-command='target remote localhost:2331' --command=#{GDB_SCRIPT} #{EXECUTABLE_NAME}.elf"
end

DEPS.each{|file| Rake::MakefileLoader.new.load(file) if File.file?(file)}
