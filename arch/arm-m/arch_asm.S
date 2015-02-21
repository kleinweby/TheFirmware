.global arch_idle_thread	
.text
.thumb_func
.type   arch_idle_thread, %function

arch_idle_thread:
	WFI
	B arch_idle_thread
