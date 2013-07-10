
ROOT="#{File.dirname(File.dirname(__FILE__))}"
TOOLCHAIN="#{ROOT}/Toolchain/arm.toolchain"

# -F should be relative to #{ROOT} ?!?
CFLAGS  = [ "-I#{ROOT}/CMSIS/inc", "-I#{ROOT}", '-integrated-as', '-mcpu=cortex-m0', '-mthumb', '-ggdb', '-fno-builtin', '-Wall', '-Werror']
CXXFLAGS  = [ *CFLAGS, '-fno-exceptions']
LDFLAGS = [ "-nostdlib", '-mcpu=cortex-m0', '-mthumb' ]
DEFINES = [ "-D__USE_CMSIS=1" ]

ENV['PATH'] = "#{TOOLCHAIN}/bin:#{File.dirname(TOOLCHAIN)}/arm-gcc.toolchain/bin:#{ENV['PATH']}"

CC      = "arm-none-eabi-clang"
LD      = "arm-none-eabi-clang"
AR      = "arm-none-eabi-ar"
OBJCOPY = "arm-none-eabi-objcopy"
STRIP   = "arm-none-eabi-strip"
GDB     = "arm-none-eabi-gdb"
NASM    = 'nasm'

OBJ_DIR = '.objs'
