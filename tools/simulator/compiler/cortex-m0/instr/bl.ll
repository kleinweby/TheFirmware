
@bl_fmt = internal unnamed_addr constant [8 x i8] c"bl %08x\00"

define internal i32 @calc_imm(i1 %s, i25 %imm10, i1 %j.1, i1 %j.2, i25 %imm11) {
  %i.1.1 = xor i1 %j.1, %s
  %i.1.2 = xor i1 %i.1.1, 1
  %i.1.3 = zext i1 %i.1.2 to i25
  %i.1.4 = shl i25 %i.1.3, 23

  %i.2.1 = xor i1 %j.2, %s
  %i.2.2 = xor i1 %i.2.1, 1
  %i.2.3 = zext i1 %i.2.2 to i25
  %i.2.4 = shl i25 %i.2.3, 22

  %s.1 = zext i1 %s to i25
  %s.2 = shl i25 %s.1, 24

  %imm.1 = or i25 %s.2, %i.1.4
  %imm.2 = or i25 %imm.1, %i.2.4
  %imm.3 = or i25 %imm.2, %imm10
  %imm.4 = or i25 %imm.3, %imm11
  %imm.5 = sext i25 %imm.4 to i32

  ret i32 %imm.5
}

; BL T1
!0 = metadata !{
  metadata !"bl",                   ; mnemonic
  i32 u0xF800D000,                  ; opcode mask
  i32 u0xF000D000,                  ; opcode
  metadata !"REG", metadata !"PC",
  metadata !"REG", metadata !"LR",
  metadata !"IMM", i8 26, i8 26,
  metadata !"IMM", i8 25, i8 16, i8 12,
  metadata !"IMM", i8 13, i8 13,
  metadata !"IMM", i8 11, i8 11,
  metadata !"IMM", i8 10, i8 0, i8 1
}

!bl.instr = !{ !0 }
define void @bl(i32* %pc, i32* %lr, i1 %s, i25 %imm10, i1 %j.1, i1 %j.2, i25 %imm11) {
  %imm = call i32 @calc_imm(i1 %s, i25 %imm10, i1 %j.1, i1 %j.2, i25 %imm11)

  %lr.1 = load i32* %pc
  %lr.2 = or i32 %lr.1, 1
  store i32 %lr.2, i32* %lr

  %pc.1 = add i32 %lr.1, %imm
  store i32 %pc.1, i32* %pc

  ret void
}
define {i8*, i32} @bl.disassemble.format({i8*, i32} %pc_val, {i8*, i32} %lr, i1 %s, i25 %imm10, i1 %j.1, i1 %j.2, i25 %imm11) {
  %pc = extractvalue {i8*, i32} %pc_val, 1
  %imm = call i32 @calc_imm(i1 %s, i25 %imm10, i1 %j.1, i1 %j.2, i25 %imm11)
  %pc.1 = add i32 %pc, %imm
  %1 = getelementptr [8 x i8]* @bl_fmt, i32 0, i32 0

  %res.1 = insertvalue {i8*, i32} undef, i8* %1, 0
  %res.2 = insertvalue {i8*, i32} %res.1, i32 %pc.1, 1
  ret {i8*, i32} %res.2
}
