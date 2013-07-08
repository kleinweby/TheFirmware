
ROOT="#{File.dirname(File.dirname(__FILE__))}"

# -F should be relative to #{ROOT} ?!?
CFLAGS  = [ "-I#{ROOT}/CMSIS/inc", '-integrated-as']
LDFLAGS = [ "-nostdlib", '-mcpu=cortex-m0', '-mthumb']
DEFINES = [ "-D__USE_CMSIS=1" ]

ENV['PATH'] = "/Users/christian/Desktop/arm-root/bin:/Users/christian/Downloads/gcc-arm-none-eabi-4_7-2013q2/bin:#{ENV['PATH']}"

CC      = "arm-none-eabi-clang"
LD      = "arm-none-eabi-gcc"
AR      = "arm-none-eabi-ar"
OBJCOPY = "arm-none-eabi-objcopy"
STRIP   = "arm-none-eabi-strip"
GDB     = "arm-none-eabi-gdb"
NASM    = 'nasm'

OBJ_DIR = '.objs'

CFLAGS << '-ggdb' << '-fno-builtin' << '-Wall' << '-mcpu=cortex-m0' << '-mthumb'
