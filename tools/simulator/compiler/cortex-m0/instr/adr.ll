; ADR T1
!0 = metadata !{
  metadata !"add",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0xA000,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"REG", metadata !"PC",
  metadata !"IMM", i8 7, i8 0, i8 2
}

!adr.instr = !{ !0 }
define void @adr(i32* %dest, i32 %pc, i10 %imm) {
  %1 = zext i10 %imm to i32
  %2 = and i32 %pc, u0xFFFFFFFC
  %3 = add i32 %2, %1
  store i32 %3, i32* %dest
  ret void
}
