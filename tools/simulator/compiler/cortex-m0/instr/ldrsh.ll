
; LDRSH (register)
!0 = metadata !{
  metadata !"ldrsh",                    ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x5E00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"REG", i8 8, i8 6 }
}

!ldrsh.instr = !{ !0 }
define void @ldrsh(i32* %dest, i16 %mem) {
  %mem.32 = sext i16 %mem to i32
  store i32 %mem.32, i32* %dest
  ret void
}
