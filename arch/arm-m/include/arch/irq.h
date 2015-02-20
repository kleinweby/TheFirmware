//
// Copyright (c) 2013, Christian Speich
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#define IRQ_NMI 2
#define IRQ_HARDFAULT 3
#define IRQ_SV 4

#define IRQ_PENDSV 5
#define IRQ_SYSTICK 6


#if ARM_M_NUMBER_OF_IRQS >= 7
#define IRQ0 7
#endif
#if ARM_M_NUMBER_OF_IRQS >= 8
#define IRQ1 8
#endif
#if ARM_M_NUMBER_OF_IRQS >= 9
#define IRQ2 9
#endif
#if ARM_M_NUMBER_OF_IRQS >= 10
#define IRQ3 10
#endif
#if ARM_M_NUMBER_OF_IRQS >= 11
#define IRQ4 11
#endif
#if ARM_M_NUMBER_OF_IRQS >= 12
#define IRQ5 12
#endif
#if ARM_M_NUMBER_OF_IRQS >= 13
#define IRQ6 13
#endif
#if ARM_M_NUMBER_OF_IRQS >= 14
#define IRQ7 14
#endif
#if ARM_M_NUMBER_OF_IRQS >= 15
#define IRQ8 15
#endif
#if ARM_M_NUMBER_OF_IRQS >= 16
#define IRQ9 16
#endif
#if ARM_M_NUMBER_OF_IRQS >= 17
#define IRQ10 17
#endif
#if ARM_M_NUMBER_OF_IRQS >= 18
#define IRQ11 18
#endif
#if ARM_M_NUMBER_OF_IRQS >= 19
#define IRQ12 19
#endif
#if ARM_M_NUMBER_OF_IRQS >= 20
#define IRQ13 20
#endif
#if ARM_M_NUMBER_OF_IRQS >= 21
#define IRQ14 21
#endif
#if ARM_M_NUMBER_OF_IRQS >= 22
#define IRQ15 22
#endif
#if ARM_M_NUMBER_OF_IRQS >= 23
#define IRQ16 23
#endif
#if ARM_M_NUMBER_OF_IRQS >= 24
#define IRQ17 24
#endif
#if ARM_M_NUMBER_OF_IRQS >= 25
#define IRQ18 25
#endif
#if ARM_M_NUMBER_OF_IRQS >= 26
#define IRQ19 26
#endif
#if ARM_M_NUMBER_OF_IRQS >= 27
#define IRQ20 27
#endif
#if ARM_M_NUMBER_OF_IRQS >= 28
#define IRQ21 28
#endif
#if ARM_M_NUMBER_OF_IRQS >= 29
#define IRQ22 29
#endif
#if ARM_M_NUMBER_OF_IRQS >= 30
#define IRQ23 30
#endif
#if ARM_M_NUMBER_OF_IRQS >= 31
#define IRQ24 31
#endif
#if ARM_M_NUMBER_OF_IRQS >= 32
#define IRQ25 32 
#endif
#if ARM_M_NUMBER_OF_IRQS >= 33
#define IRQ26 33
#endif
#if ARM_M_NUMBER_OF_IRQS >= 34
#define IRQ27 34
#endif
#if ARM_M_NUMBER_OF_IRQS >= 35
#define IRQ28 35
#endif
#if ARM_M_NUMBER_OF_IRQS >= 36
#define IRQ29 36
#endif
#if ARM_M_NUMBER_OF_IRQS >= 37
#define IRQ30 37
#endif
#if ARM_M_NUMBER_OF_IRQS >= 38
#define IRQ31 38
#endif

#define IRQ_HANDLER_MISSING IRQ_HARDFAULT
