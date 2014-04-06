; Module = 'cortex-m0'

!0 = metadata !{
  metadata !{ metadata !"r0" },
  metadata !{ metadata !"r1" },
  metadata !{ metadata !"r2" },
  metadata !{ metadata !"r3" },
  metadata !{ metadata !"r4" },
  metadata !{ metadata !"r5" },
  metadata !{ metadata !"r6" },
  metadata !{ metadata !"r7" },

  metadata !{ metadata !"r8" },
  metadata !{ metadata !"r9" },
  metadata !{ metadata !"r10" },
  metadata !{ metadata !"r11" },
  metadata !{ metadata !"r12" },

  metadata !{ metadata !"SP" },
  metadata !{ metadata !"LR" },
  metadata !{ metadata !"PC" },
  metadata !{ metadata !"XPSR" },

  ; Virtual registers
  metadata !{ metadata !"APSR", metadata !"XPSR", i32 u0xF0000000, i8 0 },
  metadata !{ metadata !"IPSR", metadata !"XPSR", i32 u0x1F, i8 0 },
  metadata !{ metadata !"EPSR", metadata !"XPSR", i32 u0x1000000, i8 0 }
}
!regs = !{!0}
@regs = internal global [17 x i32] zeroinitializer

declare i16 @cpu.fetch.i16(i32 %addr)

define {i64, i8} @cpu.fetch.instr() {
entry:
  %pc.1 = call i32 @cpu.reg.read(i8 15)
  %pc.2 = add i32 %pc.1, 2

  %instr.1 = call i16 @cpu.fetch.i16(i32 %pc.1)
  %instr.2 = zext i16 %instr.1 to i64

  %0 = and i16 %instr.1, u0xF800
  switch i16 %0, label %sixteen [
    i16 u0xF800, label %thirtytwo
    i16 u0xE800, label %thirtytwo
    i16 u0xF000, label %thirtytwo
  ]

thirtytwo:
  %instr.3 = call i16 @cpu.fetch.i16(i32 %pc.2)
  %instr.4 = zext i16 %instr.3 to i64
  %instr.5 = shl i64 %instr.2, 16
  %instr.6 = or i64 %instr.5, %instr.4

  %pc.3 = add i32 %pc.2, 2

  br label %sixteen
sixteen:
  %pc = phi i32 [ %pc.2, %entry ], [ %pc.3, %thirtytwo ]
  %instr = phi i64 [ %instr.2, %entry ], [ %instr.6, %thirtytwo ]
  %bits = phi i8 [ 16, %entry ], [ 32, %thirtytwo ]

  call void @cpu.reg.write(i8 15, i32 %pc)

  %ret.1 = insertvalue {i64, i8} undef, i64 %instr, 0
  %ret.2 = insertvalue {i64, i8} %ret.1, i8 %bits, 1

  ret {i64, i8} %ret.2
}

define i32 @cpu.reg.read(i8 %reg_num) {
  %1 = getelementptr [17 x i32]* @regs, i8 0, i8 %reg_num
  %2 = load i32* %1
  ret i32 %2
}

define void @cpu.reg.write(i8 %reg_num, i32 %val) {
entry:
  %0 = icmp eq i8 %reg_num, 15
  br i1 %0, label %pc, label %exit
pc:
  %1 = and i32 %val, u0xFFFFFFFE
  br label %exit
exit:
  %2 = phi i32 [ %val, %entry], [ %1, %pc]

  %3 = getelementptr [17 x i32]* @regs, i8 0, i8 %reg_num
  store i32 %2, i32* %3
  ret void
}

!1 = metadata !{
  metadata !{ metadata !"N", metadata !"APSR", i8 31 },
  metadata !{ metadata !"Z", metadata !"APSR", i8 30 },
  metadata !{ metadata !"C", metadata !"APSR", i8 29 },
  metadata !{ metadata !"V", metadata !"APSR", i8 28 }
}
!flags = !{!1}

; Helper Functions

define {i32, i1, i1} @addWithCarry(i32 %a, i32 %b, i32 %c)
{
  %ures.1 = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %ures.2 = extractvalue {i32, i1} %ures.1, 0
  %ures.3 = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %ures.2, i32 %c)

  %sres.1 = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %sres.2 = extractvalue {i32, i1} %sres.1, 0
  %sres.3 = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %sres.2, i32 %c)

  %1 = extractvalue {i32, i1} %ures.1, 1
  %2 = extractvalue {i32, i1} %ures.3, 1
  %3 = or i1 %1, %2

  %4 = extractvalue {i32, i1} %sres.1, 1
  %5 = extractvalue {i32, i1} %sres.3, 1
  %6 = or i1 %4, %5

  %7 = extractvalue {i32, i1} %ures.3, 0

  %ret.1 = insertvalue {i32, i1, i1} undef, i32 %7, 0
  %ret.2 = insertvalue {i32, i1, i1} %ret.1, i1 %1, 1
  %ret.3 = insertvalue {i32, i1, i1} %ret.2, i1 %6, 2

  ret {i32, i1, i1} %ret.3
}

declare {i32, i1} @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
declare {i32, i1} @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
