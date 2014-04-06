; BLX T1
!0 = metadata !{
  metadata !"blx",                   ; mnemonic
  i16 u0xFF80,                  ; opcode mask
  i16 u0x4780,                  ; opcode
  metadata !"REG", metadata !"PC",
  metadata !"REG", metadata !"LR",
  metadata !"REG", i8 6, i8 4
}

!blx.instr = !{ !0 }
define void @blx(i32* %pc, i32* %lr, i32 %new_pc) {
  %lr.1 = load i32* %pc
  %lr.2 = sub i32 %lr.1, 2 ; Don't know why, but manual says so
  %lr.3 = or i32 %lr.2, 1
  store i32 %lr.3, i32* %lr
  store i32 %new_pc, i32* %pc

  ret void
}
