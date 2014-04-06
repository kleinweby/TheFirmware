

; AND
!0 = metadata !{
  metadata !"and",                   ; mnemonic
  i16 u0xFFC0,                        ; opcode mask
  i16 u0x4000,                        ; opcode
  metadata !"REG", i8 2, i8 0,        ; out
  metadata !"REG", i8 2, i8 0,        ; in
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z"
}

!and.instr = !{ !0 }
define void @and(i32* %dest, i32 %src, i32 %src2, i1* %n, i1* %z) {
  %1 = and i32 %src, %src2
  store i32 %1, i32* %dest

  %2 = icmp slt i32 %1, 0
  store i1 %2, i1* %n

  %3 = icmp eq i32 %1, 0
  store i1 %3, i1* %z
  ret void
}
