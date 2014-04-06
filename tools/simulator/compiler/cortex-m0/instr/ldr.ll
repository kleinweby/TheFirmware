
; LDR (immediate) T1
!0 = metadata !{
  metadata !"ldr",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x6800,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"IMM", i8 10, i8 6, i8 2 }
}

; LDR (immediate) T2
!1 = metadata !{
  metadata !"ldr",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x9800,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"MEM", metadata !{ metadata !"REG", metadata !"SP", metadata !"IMM", i8 7, i8 0, i8 2 }
}

; LDR (literal)
!2 = metadata !{
  metadata !"ldr",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x4800,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"MEM", metadata !{ metadata !"REG", metadata !"PC", metadata !"IMM", i8 7, i8 0, i8 2 }
}

; LDR (register)
!3 = metadata !{
  metadata !"ldr",                    ; mnemonic
  i16 u0xFC00,                        ; opcode mask
  i16 u0x5800,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"REG", i8 8, i8 6 }
}

!ldr.instr = !{ !0, !1, !2, !3 }
define void @ldr(i32* %dest, i32 %mem) {
  store i32 %mem, i32* %dest
  ret void
}
