require_relative './BuildSupport/Common'
require 'rake/clean'
require 'rake/loaders/makefile'
require 'json'
require 'tempfile'

SRC=FileList[]

SRC.include([
	'Platform/GardenaValve/main.cc',
	'Platform/GardenaValve/Valve.cc',
])

FIRMWARE_SRC = FileList[
	# Devices
	'Firmware/Devices/MCP9800.cc',
	'Firmware/Devices/24XX64.cc',
	# General
	'Firmware/IO/OStream.cc',
	'Firmware/IO/GDBSemihostedOStream.cc',
	'Firmware/Runtime.cc',
	'Firmware/Schedule/Task.cc',
	'Firmware/Schedule/Waitable.cc',
	'Firmware/Schedule/Semaphore.cc',
	'Firmware/Schedule/Mutex.cc',
	'Firmware/Schedule/Flag.cc',
	'Firmware/Time/Systick.cc',
	'Firmware/Time/Delay.cc',
	'Firmware/Time/Timer.cc',
	'Firmware/Console/Log.cc',
	'Firmware/Console/Console.cc',
	'Firmware/Console/CLI.cc',
	'Firmware/Console/string.cc',
	'Firmware/FirmwareInfo.c',
]

require_relative 'Firmware/Target/LPC11xx/Rakefile.defs'

SRC.include(TARGET_SRC.pathmap("Firmware/Target/LPC11xx/%p"))
SRC.include(FIRMWARE_SRC)

OBJ = SRC.ext('o').pathmap("#{OBJ_DIR}/%p")
DEPS = OBJ.ext('depend')

LDFLAGS << "-lgcc"
DEFINES << "-I#{ROOT}/Firmware/Target/LPC11xx/CMSIS/inc"
CXXFLAGS << '-funsigned-char' << '-Wno-deprecated-register'

LINKER_SCRIPT = 'Firmware/Target/LPC11xx/link.ld'
GDB_SCRIPT = 'Target/startup.gdb'
EXECUTABLE_NAME = 'firmware'
JLINK_OPTIONS=ENV['JLINK_OPTIONS']

CLEAN.include(OBJ_DIR)

task :default => [ "#{EXECUTABLE_NAME}.elf" ]

file "#{EXECUTABLE_NAME}.elf" => [ *OBJ, LINKER_SCRIPT ]  do |t|
	puts " [LD]   #{t.name}"
	sh "#{LD} -T#{LINKER_SCRIPT} -o #{t.name} #{OBJ.join(' ')} #{LDFLAGS.join(' ')}"
end

task 'jlink' do |t|
	sh "/Applications/JLink/JLinkGDBServer.command -if SWD -device LPC11C24 -vd #{JLINK_OPTIONS}"
end

task 'debug' do |t|
	trap('INT') {}
	sh "#{GDB} --init-eval-command='target remote localhost:2331' --command=#{GDB_SCRIPT} #{EXECUTABLE_NAME}.elf"
end

DEPS.each{|file| Rake::MakefileLoader.new.load(file) if File.file?(file)}
