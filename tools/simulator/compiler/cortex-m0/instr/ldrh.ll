
; LDRH (immediate) T1
!0 = metadata !{
  metadata !"ldrh",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x8800,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"IMM", i8 10, i8 6, i8 1 }
}

; LDRH (register)
!1 = metadata !{
  metadata !"ldrh",                    ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x5A00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"REG", i8 8, i8 6 }
}

!ldrh.instr = !{ !0, !1 }
define void @ldrh(i32* %dest, i16 %mem) {
  %mem.32 = zext i16 %mem to i32
  store i32 %mem.32, i32* %dest
  ret void
}
