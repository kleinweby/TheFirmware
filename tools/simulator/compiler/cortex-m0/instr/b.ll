
@beq_fmt = internal unnamed_addr constant [21 x i8] c"beq %d ; pc = 0x%08x\00"
@bne_fmt = internal unnamed_addr constant [21 x i8] c"bne %d ; pc = 0x%08x\00"
@bcs_fmt = internal unnamed_addr constant [21 x i8] c"bcs %d ; pc = 0x%08x\00"
@bcc_fmt = internal unnamed_addr constant [21 x i8] c"bcc %d ; pc = 0x%08x\00"
@bmi_fmt = internal unnamed_addr constant [21 x i8] c"bmi %d ; pc = 0x%08x\00"
@bpl_fmt = internal unnamed_addr constant [21 x i8] c"bpl %d ; pc = 0x%08x\00"
@bvs_fmt = internal unnamed_addr constant [21 x i8] c"bvs %d ; pc = 0x%08x\00"
@bvc_fmt = internal unnamed_addr constant [21 x i8] c"bvc %d ; pc = 0x%08x\00"
@bhi_fmt = internal unnamed_addr constant [21 x i8] c"bhi %d ; pc = 0x%08x\00"
@bls_fmt = internal unnamed_addr constant [21 x i8] c"bls %d ; pc = 0x%08x\00"
@bge_fmt = internal unnamed_addr constant [21 x i8] c"bge %d ; pc = 0x%08x\00"
@blt_fmt = internal unnamed_addr constant [21 x i8] c"blt %d ; pc = 0x%08x\00"
@bgt_fmt = internal unnamed_addr constant [21 x i8] c"bgt %d ; pc = 0x%08x\00"
@ble_fmt = internal unnamed_addr constant [21 x i8] c"ble %d ; pc = 0x%08x\00"
@b_fmt = internal unnamed_addr constant [19 x i8] c"b %d ; pc = 0x%08x\00"

define i1 @condition_passed(i4 %cond, i1 %z, i1 %n, i1 %c, i1 %v) {
  switch i4 %cond, label %false [
    i4 0, label %eq
    i4 1, label %ne
    i4 2, label %cs
    i4 3, label %cc
    i4 4, label %mi
    i4 5, label %pl
    i4 6, label %vs
    i4 7, label %vc
    i4 8, label %hi
    i4 9, label %ls
    i4 10, label %ge
    i4 11, label %lt
    i4 12, label %gt
    i4 13, label %le
    i4 14, label %always
  ]

eq: ; Z == 1
  ret i1 %z

ne: ; Z == 0
  %1 = xor i1 %z, 1
  ret i1 %1

cs: ; C==1
  ret i1 %c

cc: ; C==0
  %2 = xor i1 %c, 1
  ret i1 %2

mi: ; N==1
  ret i1 %n

pl: ; N==0
  %3 = xor i1 %n, 1
  ret i1 %3

vs: ; V==1
  ret i1 %v

vc: ; V==0
  %4 = xor i1 %v, 1
  ret i1 %4

hi: ; C == 1 and Z == 0
  %5 = xor i1 %z, 1
  %6 = and i1 %c, %5
  ret i1 %6

ls: ; C == 0 or Z == 1
  %7 = xor i1 %c, 1
  %8 = or i1 %z, %7
  ret i1 %8

ge: ; N == V
  %9 = xor i1 %n, %v
  %10 = xor i1 %9, 1
  ret i1 %10

lt: ; N != V
  %11 = xor i1 %n, %v
  ret i1 %11

gt: ; Z == 0 and N == V
  %12 = xor i1 %c, 1
  %13 = xor i1 %n, %v
  %14 = xor i1 %13, 1
  %15 = and i1 %12, %14
  ret i1 %15

le: ; Z == 1 or N != V
  %16 = xor i1 %n, %v
  %17 = or i1 %z, %16
  ret i1 %17

always:
  ret i1 1

false:
  ret i1 0
}

; B(2) unconditional branch
!0 = metadata !{
  metadata !"b",                   ; mnemonic
  i16 u0xF000,                   ; opcode mask
  i16 u0xD000,                   ; opcode
  metadata !"REG", metadata !"PC",
  metadata !"IMM", i8 11, i8 8,
  metadata !"IMM", i8 7, i8 0,
  metadata !"FLAG", metadata !"Z",
  metadata !"FLAG", metadata !"C",
  metadata !"FLAG", metadata !"N",
  metadata !"FLAG", metadata !"V"
}

!b.instr = !{ !0 }
define void @b(i32* %pc, i4 %cond, i8 %imm, i1 %z, i1 %c, i1 %n, i1 %v) {
  %passed = call i1 @condition_passed(i4 %cond, i1 %z, i1 %n, i1 %c, i1 %v)
  br i1 %passed, label %jump, label %exit
jump:
  %imm.11 = sext i8 %imm to i11
  call void @b2(i32* %pc, i11 %imm.11)
  br label %exit
exit:
  ret void
}
define {i8*, i32, i32} @b.disassemble.format({i8*, i32} %pc_val, i4 %cond, i8 %imm, i1 %z, i1 %c, i1 %n, i1 %v) {
  %pc = extractvalue {i8*, i32} %pc_val, 1
  %1 = sext i8 %imm to i32
  %2 = shl i32 %1, 1
  %3 = add i32 %2, %pc
  %4 = add i32 %3, 2

  switch i4 %cond, label %always [
    i4 0, label %eq
    i4 1, label %ne
    i4 2, label %cs
    i4 3, label %cc
    i4 4, label %mi
    i4 5, label %pl
    i4 6, label %vs
    i4 7, label %vc
    i4 8, label %hi
    i4 9, label %ls
    i4 10, label %ge
    i4 11, label %lt
    i4 12, label %gt
    i4 13, label %le
  ]

eq:
  %5 = getelementptr [21 x i8]* @beq_fmt, i32 0, i32 0
  br label %exit
ne:
  %6 = getelementptr [21 x i8]* @bne_fmt, i32 0, i32 0
  br label %exit
cs:
  %7 = getelementptr [21 x i8]* @bcs_fmt, i32 0, i32 0
  br label %exit
cc:
  %8 = getelementptr [21 x i8]* @bcc_fmt, i32 0, i32 0
  br label %exit
mi:
  %9 = getelementptr [21 x i8]* @bmi_fmt, i32 0, i32 0
  br label %exit
pl:
  %10 = getelementptr [21 x i8]* @bpl_fmt, i32 0, i32 0
  br label %exit
vs:
  %11 = getelementptr [21 x i8]* @bvs_fmt, i32 0, i32 0
  br label %exit
vc:
  %12 = getelementptr [21 x i8]* @bvc_fmt, i32 0, i32 0
  br label %exit
hi:
  %13 = getelementptr [21 x i8]* @bhi_fmt, i32 0, i32 0
  br label %exit
ls:
  %14 = getelementptr [21 x i8]* @bls_fmt, i32 0, i32 0
  br label %exit
ge:
  %15 = getelementptr [21 x i8]* @bge_fmt, i32 0, i32 0
  br label %exit
lt:
  %16 = getelementptr [21 x i8]* @blt_fmt, i32 0, i32 0
  br label %exit
gt:
  %17 = getelementptr [21 x i8]* @bgt_fmt, i32 0, i32 0
  br label %exit
le:
  %18 = getelementptr [21 x i8]* @ble_fmt, i32 0, i32 0
  br label %exit
always:
  %19 = getelementptr [19 x i8]* @b_fmt, i32 0, i32 0
  br label %exit
exit:
  %20 = phi i8* [ %5 , %eq ], [ %6, %ne ], [ %7, %cs ], [ %8, %cc ], [ %9, %mi ], [ %10, %pl ], [ %11, %vs ], [ %12, %vc ], [ %13, %hi ], [ %14, %ls ], [ %15, %ge ], [ %16, %lt ], [ %17, %gt ], [ %18, %le ], [ %19, %always ]

  %res.1 = insertvalue {i8*, i32, i32} undef, i8* %20, 0
  %res.2 = insertvalue {i8*, i32, i32} %res.1, i32 %2, 1
  %res.3 = insertvalue {i8*, i32, i32} %res.2, i32 %4, 2
  ret {i8*, i32, i32} %res.3
}

; B(2) unconditional branch
!1 = metadata !{
  metadata !"b",                   ; mnemonic
  i16 u0xF000,                   ; opcode mask
  i16 u0xE000,                   ; opcode
  metadata !"REG", metadata !"PC",
  metadata !"IMM", i8 10, i8 0
}

!b2.instr = !{ !1 }
define void @b2(i32* %pc, i11 %imm) {
  %1 = sext i11 %imm to i32
  %2 = shl i32 %1, 1
  %3 = load i32* %pc
  %4 = add i32 %2, %3
  %5 = add i32 %4, 2
  store i32 %5, i32* %pc
  ret void
}
define {i8*, i32, i32} @b2.disassemble.format({i8*, i32} %pc_val, i11 %imm) {
  %pc = extractvalue {i8*, i32} %pc_val, 1
  %1 = sext i11 %imm to i32
  %2 = shl i32 %1, 1
  %3 = add i32 %2, %pc
  %4 = add i32 %3, 2
  %5 = getelementptr [19 x i8]* @b_fmt, i32 0, i32 0

  %res.1 = insertvalue {i8*, i32, i32} undef, i8* %5, 0
  %res.2 = insertvalue {i8*, i32, i32} %res.1, i32 %2, 1
  %res.3 = insertvalue {i8*, i32, i32} %res.2, i32 %4, 2
  ret {i8*, i32, i32} %res.3
}
