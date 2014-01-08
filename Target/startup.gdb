monitor device LPC11C24
monitor flash download 1
monitor flash breakpoints 1
monitor reset
load
#source semihosted.py
monitor reset
break main
#break TheFirmware::Runtime::AssertHandler(char const*, char const*, unsigned int, char const*, char const*)