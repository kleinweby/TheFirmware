; CMP (immediate) T1
!0 = metadata !{
  metadata !"cmp",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x2800,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"IMM", i8 7, i8 0,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!cmp.instr = !{ !0 }
define void @cmp(i32 %a, i8 %imm, i1* %n, i1* %z, i1* %c, i1* %v) {
  %imm.32 = zext i8 %imm to i32
  call void @cmp2(i32 %a, i32 %imm.32, i1* %n, i1* %z, i1* %c, i1* %v)
  ret void
}

; CMP (register) T1
!1 = metadata !{
  metadata !"cmp",                   ; mnemonic
  i16 u0xFFC0,                        ; opcode mask
  i16 u0x4280,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}
; CMP (register) T2
!2 = metadata !{
  metadata !"cmp",                   ; mnemonic
  i16 u0xFF00,                        ; opcode mask
  i16 u0x4500,                        ; opcode
  metadata !"REG", i8 7, i8 7, i8 2, i8 0,
  metadata !"REG", i8 6, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!cmp2.instr = !{ !1, !2 }
define void @cmp2(i32 %a, i32 %b, i1* %n, i1* %z, i1* %c, i1* %v) {
  %b.not = xor i32 %b, u0xFFFFFFFF
  %res = call {i32, i1, i1} @addWithCarry(i32 %a, i32 %b.not, i32 0)
  %1 = extractvalue {i32, i1, i1} %res, 0

  ; Updating flags
  %2 = icmp slt i32 %1, 0
  store i1 %2, i1* %n

  %3 = icmp eq i32 %1, 0
  store i1 %3, i1* %z

  %4 = extractvalue {i32, i1, i1} %res, 1
  store i1 %4, i1* %c

  %5 = extractvalue {i32, i1, i1} %res, 2
  store i1 %5, i1* %v

  ret void
}
