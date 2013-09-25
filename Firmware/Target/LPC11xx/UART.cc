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

#include "UART.h"

#include "LPC11xx.h"

namespace TheFirmware {
namespace LPC11xx {

#define IER_RBR         (0x01<<0)
#define IER_THRE        (0x01<<1)
#define IER_RLS         (0x01<<2)
#define IER_ABEO        (0x01<<8)
#define IER_ABTO        (0x01<<9)

#define LSR_RDR         (0x01<<0)
#define LSR_OE          (0x01<<1)
#define LSR_PE          (0x01<<2)
#define LSR_FE          (0x01<<3)
#define LSR_BI          (0x01<<4)
#define LSR_THRE        (0x01<<5)
#define LSR_TEMT        (0x01<<6)
#define LSR_RXFE        (0x01<<7)

UART* GlobalUART;

enum {
  kIIRReceiveLineStatus = 0x6,
  kIIRReceiveDataAvailable = 0x4
};

extern "C" void UART_IRQHandler(void)
{
  GlobalUART->isr();
}

UART::UART(uint32_t baud)
{
    // Configure RX PIN
    LPC_IOCON->PIO1_6 &= ~0x07;
    LPC_IOCON->PIO1_6 |= 0x01;
    // Configure TX PIN
    LPC_IOCON->PIO1_7 &= ~0x07;	
    LPC_IOCON->PIO1_7 |= 0x01;

    // Enable Clock (divider 1)
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<12);
    LPC_SYSCON->UARTCLKDIV = 0x1;  

    // 8 bits, no Parity, 1 Stop bit
    LPC_UART->LCR = 0x83;

    // Configure Baudrate
    {
        uint32_t fdiv = ((SystemCoreClock/LPC_SYSCON->UARTCLKDIV)/16)/baud;

        LPC_UART->DLM = fdiv / 256;                         
        LPC_UART->DLL = fdiv % 256;
        LPC_UART->FDR = 0x10; // Default
    }

    LPC_UART->LCR = 0x03;       /* DLAB = 0 */
  LPC_UART->FCR = 0x07;     /* Enable and reset TX and RX FIFO. */

  /* Read to clear the line status. */
  uint32_t regVal = LPC_UART->LSR;

  /* Ensure a clean start, no data in either TX or RX FIFO. */
  while (( LPC_UART->LSR & (LSR_THRE|LSR_TEMT)) != (LSR_THRE|LSR_TEMT) );
  while ( LPC_UART->LSR & LSR_RDR )
  {
    regVal = LPC_UART->RBR; /* Dump data from RX FIFO */
  }

  GlobalUART = this;

  LPC_UART->IER = IER_RBR | IER_RLS;

  // Enable interrupt
  NVIC_EnableIRQ(UART_IRQn);
}

void UART::put(char c)
{
    while ( !(LPC_UART->LSR & LSR_THRE) )
        ;
    LPC_UART->THR = c;
}

char UART::get()
{
  char c;

  this->rxBuffer.get(c);

  return c;
}

void UART::isr()
{
  uint8_t IIR = LPC_UART->IIR & 0xE;

  if (IIR == kIIRReceiveLineStatus) {
    uint8_t LSR = LPC_UART->LSR;

    if (LSR & (LSR_OE | LSR_PE | LSR_FE | LSR_RXFE | LSR_BI))
    {
      uint8_t d = LPC_UART->RBR;
#pragma unused(d)
      return;
    }

    // No Error, read data
    if (LSR & LSR_RDR)    
      this->rxBuffer.put(LPC_UART->RBR, true);
  }
  else if (IIR == kIIRReceiveDataAvailable) /* Receive Data Available */
    this->rxBuffer.put(LPC_UART->RBR, true);
}

} // namespace LPC11xx
} // namespace TheFirmware
