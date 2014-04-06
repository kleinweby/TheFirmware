

; BIC
!0 = metadata !{
  metadata !"bics",                   ; mnemonic
  i16 u0xFFC0,                        ; opcode mask
  i16 u0x4380,                        ; opcode
  metadata !"REG", i8 2, i8 0,        ; out
  metadata !"REG", i8 2, i8 0,        ; in
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z"
}

!bic.instr = !{ !0 }
define void @bic(i32* %dest, i32 %src, i32 %src2, i1* %n, i1* %z) {
  %1 = xor i32 %src2, -1 ; ~src2
  %2 = and i32 %src, %1
  store i32 %2, i32* %dest

  %3 = icmp slt i32 %2, 0
  store i1 %3, i1* %n

  %4 = icmp eq i32 %2, 0
  store i1 %4, i1* %z
  ret void
}
