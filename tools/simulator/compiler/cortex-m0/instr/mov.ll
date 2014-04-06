

; MOV (immediate) T1
!0 = metadata !{
  metadata !"movs",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x2000,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"IMM", i8 7, i8 0,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z"
}

!mov.instr = !{ !0 }
define void @mov(i32* %dest, i8 %imm, i1* %n, i1* %z) {
  %1 = zext i8 %imm to i32
  store i32 %1, i32* %dest

  %2 = icmp slt i32 %1, 0
  store i1 %2, i1* %n

  %3 = icmp eq i32 %1, 0
  store i1 %3, i1* %z
  ret void
}

; MOV (register) T1
!1 = metadata !{
  metadata !"mov",                   ; mnemonic
  i16 u0xFF00,                        ; opcode mask
  i16 u0x4600,                        ; opcode
  metadata !"REG", i8 7, i8 7, i8 2, i8 0,
  metadata !"REG", i8 6, i8 3
}

!mov2.instr = !{ !1 }
define void @mov2(i32* %dest, i32 %src) {
  store i32 %src, i32* %dest
  ret void
}

; MOV (register) T2
!2 = metadata !{
  metadata !"movs",                   ; mnemonic
  i16 u0xFFC0,                        ; opcode mask
  i16 u0x0000,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z"
}

!mov3.instr = !{ !2 }
define void @mov3(i32* %dest, i32 %src, i1* %n, i1* %z) {
  store i32 %src, i32* %dest

  %1 = icmp slt i32 %src, 0
  store i1 %1, i1* %n

  %2 = icmp eq i32 %src, 0
  store i1 %2, i1* %z
  ret void
}

