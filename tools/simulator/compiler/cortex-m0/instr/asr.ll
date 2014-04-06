

; ASR(1) two register immediate
!0 = metadata !{
  metadata !"asrs",                   ; mnemonic
  i16 u0xF800,                        ; opcode mask
  i16 u0x1000,                        ; opcode
  metadata !"REG", i8 2, i8 0,        ; out
  metadata !"REG", i8 5, i8 3,        ; in
  metadata !"IMM", i8 11, i8 6,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C"
}

!asr.instr = !{ !0 }
define void @asr(i32* %dest, i32 %src, i7 %shift, i1* %n, i1* %z, i1* %c) {
  %1 = zext i7 %shift to i32

  call void @asr2(i32* %dest, i32 %src, i32 %1, i1* %n, i1* %z, i1* %c)

  ret void
}

; ASR(2) two register
!1 = metadata !{
  metadata !"asrs",                   ; mnemonic
  i16 u0xFF80,                        ; opcode mask
  i16 u0x4100,                        ; opcode
  metadata !"REG", i8 2, i8 0,        ; out
  metadata !"REG", i8 2, i8 0,        ; in
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C"
}
!asr2.instr = !{!1}
define void @asr2(i32* %dest, i32 %src, i32 %shift, i1* %n, i1* %z, i1* %c) {
  %1 = icmp eq i32 %shift, 0
  br i1 %1, label %zero, label %non_zero
zero:
  %2 = icmp slt i32 %src, 0
  br i1 %2, label %neg, label %non_neg
neg:
  %3 = and i32 -1, -1 ; result is -1
  store i1 1, i1* %c
  br label %exit
non_neg:
  %4 = and i32 0, 0 ; result is 0
  store i1 0, i1* %c
  br label %exit
non_zero:
  %5 = lshr i32 %src, %shift
  %6 = trunc i32 %5 to i1
  store i1 %6, i1* %c

  %7 = ashr i32 %src, %shift

  br label %exit
exit:
  %res = phi i32 [ %3, %neg ], [ %4, %non_neg ], [ %7, %non_zero ]
  store i32 %res, i32* %dest

  %n.val = icmp slt i32 %res, 0
  store i1 %n.val, i1* %n

  %z.val = icmp eq i32 %res, 0
  store i1 %z.val, i1* %z

  ret void
}
