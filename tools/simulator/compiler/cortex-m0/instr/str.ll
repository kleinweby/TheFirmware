; STR (immediate) T1
!0 = metadata !{
  metadata !"str",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x6000,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"IMM", i8 10, i8 6, i8 2 }
}

; LDR (immediate) T2
!1 = metadata !{
  metadata !"str",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x9000,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"MEM", metadata !{ metadata !"REG", metadata !"SP", metadata !"IMM", i8 7, i8 0, i8 2 }
}

; STR (register) T1
!2 = metadata !{
  metadata !"str",                    ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x5000,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"MEM", metadata !{ metadata !"REG", i8 5, i8 3, metadata !"REG", i8 8, i8 6 }
}

!str.instr = !{ !0, !1, !2 }
define void @str(i32 %src, i32* %mem) {
  store i32 %src, i32* %mem
  ret void
}