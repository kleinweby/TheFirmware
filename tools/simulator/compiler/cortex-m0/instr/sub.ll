; SUB (immediate) T1
!0 = metadata !{
  metadata !"sub",                    ; mnemonic
  i16 u0xFC00,                        ; opcode mask
  i16 u0x1C00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"IMM", i8 8, i8 6,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

; SUB (immediate) T2
!1 = metadata !{
  metadata !"sub",                    ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x3800,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"REG", i8 10, i8 8,
  metadata !"IMM", i8 7, i8 0,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

; SUB (register) T2
!2 = metadata !{
  metadata !"sub",                    ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x1A00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"REG", i8 8, i8 6,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!sub.instr = !{ !0, !1, !2 }
define void @sub(i32* %dest, i32 %src, i32 %src2, i1* %n, i1* %z, i1* %c, i1* %v) {
  %1 = xor i32 %src2, u0xFFFFFFFF
  %res = call {i32, i1, i1} @addWithCarry(i32 %src, i32 %1, i32 0)
  %2 = extractvalue {i32, i1, i1} %res, 0
  store i32 %2, i32* %dest

  ; Updating flags
  %3 = icmp slt i32 %2, 0
  store i1 %3, i1* %n

  %4 = icmp eq i32 %2, 0
  store i1 %4, i1* %z

  %5 = extractvalue {i32, i1, i1} %res, 1
  store i1 %5, i1* %c

  %6 = extractvalue {i32, i1, i1} %res, 2
  store i1 %6, i1* %v

  ret void
}

; SUB (SP minus immediate) T2
!3 = metadata !{
  metadata !"sub",                    ; mnemonic
  i16 u0xFF80,                        ; opcode mask
  i16 u0xB080,                        ; opcode
  metadata !"REG", metadata !"SP",
  metadata !"REG", metadata !"SP",
  metadata !"IMM", i8 6, i8 0, i8 2
}

!sub2.instr = !{ !3 }
define void @sub2(i32* %dest, i32 %src, i32 %src2) {
  %1 = xor i32 %src2, u0xFFFFFFFF
  %res = call {i32, i1, i1} @addWithCarry(i32 %src, i32 %1, i32 0)
  %2 = extractvalue {i32, i1, i1} %res, 0
  store i32 %2, i32* %dest

  ret void
}
