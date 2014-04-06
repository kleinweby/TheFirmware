
; LDRB (immediate) T1
!0 = metadata !{
  metadata !"ldrb",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x7800,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"IMM", i8 10, i8 6 }
}

; LDRB (register)
!1 = metadata !{
  metadata !"ldrb",                    ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x5C00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"REG", i8 8, i8 6 }
}

!ldrb.instr = !{ !0, !1 }
define void @ldrb(i32* %dest, i8 %mem) {
  %mem.32 = zext i8 %mem to i32
  store i32 %mem.32, i32* %dest
  ret void
}
