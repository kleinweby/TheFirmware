; CMN (register)
!0 = metadata !{
  metadata !"cmn",                   ; mnemonic
  i16 u0xFFC0,                        ; opcode mask
  i16 u0x42C0,                        ; opcode
  metadata !"REG", i8 2, i8 0,
  metadata !"REG", i8 5, i8 3,
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"V"
}

!cmn.instr = !{ !0 }
define void @cmn(i32 %a, i32 %b, i1* %n, i1* %z, i1* %c, i1* %v) {
  %res = call {i32, i1, i1} @addWithCarry(i32 %a, i32 %b, i32 0)
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
