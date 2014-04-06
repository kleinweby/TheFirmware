
; MSR APSR
!0 = metadata !{
  metadata !"msr",                   ; mnemonic
  i32 u0xFFE0D0FF,                   ; opcode mask
  i32 u0xF3808000,                   ; opcode
  metadata !"REG", metadata !"APSR",
  metadata !"REG", i8 18, i8 16
}

!msr.instr = !{ !0 }
define void @msr(i32* %dest, i32 %src) {
  store i32 %src, i32* %dest
  ret void
}
