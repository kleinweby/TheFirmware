; BX T1
!0 = metadata !{
  metadata !"bx",                   ; mnemonic
  i16 u0xFF80,                  ; opcode mask
  i16 u0x4700,                  ; opcode
  metadata !"REG", metadata !"PC",
  metadata !"REG", i8 6, i8 4
}

!bx.instr = !{ !0 }
define void @bx(i32* %pc, i32 %new_pc) {
  store i32 %new_pc, i32* %pc

  ret void
}
