; ModuleID = 'adds'

; ADD(1) small immediate two registers
!0 = metadata !{
  metadata !"adds",                   ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x1C00,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"IMM", i8 8, i8 6,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!add1.instr = !{ !0 }
define void @add1(i32* %dest, i32 %src, i3 %imm, i1* %n, i1* %z, i1* %c, i1* %v) {
  %1 = zext i3 %imm to i32
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

; ADD(2) big immediate one register
!2 = metadata !{
  metadata !"adds",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x3000,                        ; opcode
  metadata !"REG", i8 10, i8 8,       ; reg in
  metadata !"REG", i8 10, i8 8,       ; reg out
  metadata !"IMM", i8 7, i8 0,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!add2.instr = !{ !2 }
define void @add2(i32* %dest, i32 %src, i8 %imm, i1* %n, i1* %z, i1* %c, i1* %v) {
  %1 = zext i8 %imm to i32
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

; ADD(3) three registers T1
!3 = metadata !{
  metadata !"adds",                   ; mnemonic
  i16 u0xFE00,                        ; opcode mask
  i16 u0x1800,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"REG", i8 8, i8 6,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!add3.instr = !{ !3 }
define void @add3(i32* %dest, i32 %src1, i32 %src2, i1* %n, i1* %z, i1* %c, i1* %v) {
  %res = call {i32, i1, i1} @addWithCarry(i32 %src1, i32 %src2, i32 0)
  %1 = extractvalue {i32, i1, i1} %res, 0
  store i32 %1, i32* %dest

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

; ADD(3) (register) T2
!4 = metadata !{
  metadata !"add",                   ; mnemonic
  i16 u0xFF00,                        ; opcode mask
  i16 u0x4400,                        ; opcode
  metadata !"REG", i8 7, i8 7, i8 2, i8 0,
  metadata !"REG", i8 6, i8 3,
  metadata !"IMM", i8 7, i8 7, i8 2, i8 0,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!add4.instr = !{ !4 }
define void @add4(i32* %dest, i32 %src2, i4 %dest_reg_num, i1* %n, i1* %z, i1* %c, i1* %v) {
  %src1 = load i32* %dest
  %res = call {i32, i1, i1} @addWithCarry(i32 %src1, i32 %src2, i32 0)
  %1 = extractvalue {i32, i1, i1} %res, 0
  store i32 %1, i32* %dest

  %cmp = icmp ne i4 %dest_reg_num, 15
  br i1 %cmp, label %update_flags, label %exit

update_flags:
  ; Updating flags
  %2 = icmp slt i32 %1, 0
  store i1 %2, i1* %n

  %3 = icmp eq i32 %1, 0
  store i1 %3, i1* %z

  %4 = extractvalue {i32, i1, i1} %res, 1
  store i1 %4, i1* %c

  %5 = extractvalue {i32, i1, i1} %res, 2
  store i1 %5, i1* %v

  br label %exit
exit:
  ret void
}

; ADD(6) rd = sp plus immediate
!6 = metadata !{
  metadata !"adds",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0xA800,                        ; opcode
  metadata !"REG", i8 10, i8 8,
  metadata !"IMM", i8 7, i8 0, i8 2,
  metadata !"REG", metadata !"SP"
}

; ADD(7) sp plus immediate
!7 = metadata !{
  metadata !"adds",                   ; mnemonic
  i16 u0xFF80,                        ; opcode mask
  i16 u0xB000,                        ; opcode
  metadata !"REG", metadata !"SP",
  metadata !"IMM", i8 6, i8 0, i8 2,
  metadata !"REG", metadata !"SP"
}

!add6.instr = !{ !6, !7 }
define void @add6(i32* %dest, i10 %imm, i32 %sp) {
  %1 = zext i10 %imm to i32
  %2 = add i32 %sp, %1
  store i32 %2, i32* %dest
  ret void
}
