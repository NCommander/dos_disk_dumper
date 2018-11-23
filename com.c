/*
 *  Listing 3  -  COM.C  -  "RS-232 Interrupts The C Way"
 *
 *  Copyright (C) Mark R. Nelson 1990
 *
 * This file contains all the code to implement a complete
 * interrupt driven interface to one of the RS-232 COM
 * ports on an IBM compatible PC.
 */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "com.h"
/*
 * This group of defines creates all the definitions used
 * to access registers and bit fields in the 8250 UART.
 * While the names may not be the best, they are the ones
 * used in the 8250 data sheets, so they are generally not
 * changed.  Since the definitions are only used in COM.C,
 * they are not included in the COM.H header file where
 * they might normally be expected.
 */
#define RBR              0    /* Receive buffer register */
#define THR              0    /* Transmit holding reg.   */
#define IER              1    /* Interrupt Enable reg.   */
#define IER_RX_DATA      1    /* Enable RX interrupt bit */
#define IER_THRE         2    /* Enable TX interrupt bit */
#define IIR              2    /* Interrupt ID register   */
#define IIR_MODEM_STATUS 0    /* Modem stat. interrupt ID*/
#define IIR_TRANSMIT     2    /* Transmit interrupt ID   */
#define IIR_RECEIVE      4    /* Receive interrupt ID    */
#define IIR_LINE_STATUS  6    /* Line stat. interrupt ID */
#define LCR              3    /* Line control register   */
#define LCR_DLAB         0x80 /* Divisor access bit      */
#define LCR_EVEN_PARITY  0x8  /* Set parity 'E' bits     */
#define LCR_ODD_PARITY   0x18 /* Set parity 'O' bits     */
#define LCR_NO_PARITY    0    /* Set parity 'N' bits     */
#define LCR_1_STOP_BIT   0    /* Bits to set 1 stop bit  */
#define LCR_2_STOP_BITS  4    /* Bits to set 2 stop bits */
#define LCR_5_DATA_BITS  0    /* Bits to set 5 data bits */
#define LCR_6_DATA_BITS  1    /* Bits to set 6 data bits */
#define LCR_7_DATA_BITS  2    /* Bits to set 7 data bits */
#define LCR_8_DATA_BITS  3    /* Bits to set 8 data bits */
#define MCR              4    /* Modem control register  */
#define MCR_DTR          1    /* Bit to turn on DTR      */
#define MCR_RTS          2    /* Bit to turn on RTS      */
#define MCR_OUT1         4    /* Bit to turn on OUT1     */
#define MCR_OUT2         8    /* Bit to turn on OUT2     */
#define LSR              5    /* Line Status register    */
#define MSR              6    /* Modem Status register   */
#define DLL              0    /* Divisor latch LSB       */
#define DLM              1    /* Divisor latch MSB       */
/*
 * Various constants used only in this program.
 */
#define INT_CONTROLLER   0x20  /* The address of the 8259*/
#define EOI              0x20  /* The end of int command */
#define BREAK_VECTOR     0x23  /* The CTRL-BREAK vector  */

/*
 * These are two static variables used in COM.C.  com is
 * the pointer to the port that will be serviced by the
 * ISR.  The old break handler points to the CTRL-BREAK
 * handler that was in place before the port was opened.
 * It will be restored when the port is closed.
 */
static PORT *com = NULL;
static void ( interrupt far * old_break_handler )();

/*
 * This routine intercepts the CTRL-BREAK vector.  This
 * prevents the program from terminating before having a
 * chance to turn off COM interrupts.  This handler does
 * nothing, but it could be used to set a flag indicating
 * it is time to abort.
 */
void interrupt far break_handler()
{

}
/*
 * This is the interrupt service routine for the COM port.
 * It sits in a loop reading the interrrupt ID register, then
 * servicing one of the four different types of interrupts.
 * Note that we shouldn't even get Modem Status and Line
 * interrupts in this implementation, but they are left
 * in for later enhancements.
 */
static void interrupt far interrupt_service_routine()
{
  unsigned char c;

  enable();
  for ( ; ; ) {
    switch ( inportb( com->uart_base + IIR ) ) {
/*
 * If the present interrupt is due to a modem status line
 * change, the MSR is read to clear the interrupt, but
 * nothing else is done.
 */
      case IIR_MODEM_STATUS :
        inportb( com->uart_base + MSR );
        break;
/*
 * If the interrupt is due to the transmit holding register
 * being ready for another character, I first check to see
 * if any characters are left in the output buffer.  If
 * not, Transmit interrupts are disabled.  Otherwise, the
 * next character is extracted from the buffer and written
 * to the UART.
 */
      case IIR_TRANSMIT :
        if ( com->out.read_index == com->out.write_index )
          outportb( com->uart_base + IER, IER_RX_DATA );
        else {
          c = com->out.buffer[ com->out.read_index++ ];
          outportb( com->uart_base + THR, c );
        }
        break;
/*
 * When a new character comes in and generates an
 * interrupt, it is read in.  If there is room in the input
 * buffer, it is stored, otherwise it is discarded.
 */
      case IIR_RECEIVE :
        c = (unsigned char) inportb( com->uart_base+RBR );
        if ((com->in.write_index+1 ) != com->in.read_index)
            com->in.buffer[ com->in.write_index++ ] =  c ;
        break;
/*
 * All this code does is read the Line Status register, to
 * clear the source of the interrupt.
 */
      case IIR_LINE_STATUS :
        inportb( com->uart_base + LSR );
        break;
/*
 * If there are no valid interrupts left to service, an EOI
 * is written to the 8259 interrupt controller, and the
 * routine exits.
 */
      default :
        outportb( INT_CONTROLLER, EOI );
        return;
    }
  }
}
/*
 * This routine opens an RS-232 port up.  This means it
 * allocates space for a PORT strcture, initializes the
 * input and output buffers, stores the uart address and
 * the interrupt number.  It then gets and stored the
 * interrupt vector presently set up for the UART, then
 * installs its own.  It also sets up a handler to
 * intercept the CTRL-BREAK handler.  Finally, it tells the
 * 8259 interrupt controller to begin accepting interrupts
 * on the IRQ line used by this COM port.
 */
PORT *port_open( int address, int int_number )
{
  unsigned char temp;
  PORT *port;

  if ((port = malloc( sizeof( PORT ))) == NULL)
    return( NULL );
  com = port;
  port->in.write_index = port->in.read_index = 0;
  port->out.write_index = port->out.read_index = 0;
  port->uart_base = address;
  port->irq_mask = (char) 1 << (int_number % 8 );
  port->interrupt_number = int_number;
  port->old_vector = getvect( int_number );
  setvect( int_number, interrupt_service_routine );
  old_break_handler = getvect( BREAK_VECTOR );
  setvect( BREAK_VECTOR, break_handler );
  temp = (char) inportb( INT_CONTROLLER + 1 );
  outportb( INT_CONTROLLER + 1, ~port->irq_mask & temp );
  return( port );
}
/*
 * This routine establishes the operating parameters for a
 * port after it has been opened.  This means it sets the
 * baud rate, parity, number of data bits, and number of
 * stop bits.  Interrupts are disabled before the routine
 * starts changing registers, and are then reenabled after
 * the changes are complete.
 */
void port_set( PORT *port,
               long speed,
               char parity,
               int data,
               int stopbits )
{
  unsigned char lcr_out;
  unsigned char mcr_out;
  unsigned char low_divisor;
  unsigned char high_divisor;
/*
 * First disable all interrupts from the port.  I also read
 * RBR just in case their is a char sitting there ready to
 * generate an interupt.
 */
  outportb( port->uart_base + IER, 0 );
  inportb( port->uart_base );
/*
 * Writing the baud rate means first enabling the divisor
 * latch registers, then writing the 16 bit divisor int
 * two steps, then disabling the divisor latch so the other
 * registers can be accessed normally.
 */
  low_divisor = (char) (115200L / speed ) & 0xff;
  high_divisor = (char) ((115200L /  speed ) >> 8);
  outportb( port->uart_base + LCR, LCR_DLAB );
  outportb( port->uart_base + DLL, low_divisor );
  outportb( port->uart_base + DLM, high_divisor );
  outportb( port->uart_base + LCR, 0 );
/*
 * Setting up the line control register establishes the
 * parity, number of bits, and number of stop bits.
 */
  if ( parity== 'E' )
    lcr_out = LCR_EVEN_PARITY;
  else if ( parity == 'O' )
    lcr_out = LCR_ODD_PARITY;
  else
    lcr_out = LCR_NO_PARITY;

  if ( stopbits == 2 )
    lcr_out |= LCR_2_STOP_BITS;

  if ( data == 6 )
    lcr_out |= LCR_6_DATA_BITS;
  else if ( data == 7 )
    lcr_out |= LCR_7_DATA_BITS;
  else if ( data == 8 )
    lcr_out |= LCR_8_DATA_BITS;

  outportb( port->uart_base + LCR, lcr_out );
/*
 * I turn on RTS and DTR, as well as OUT2.  OUT2 is needed
 * to allow interrupts on PC compatible cards.
 */
  mcr_out = MCR_RTS | MCR_DTR | MCR_OUT2 ;
  outportb( port->uart_base + MCR, mcr_out );
/*
 * Finally, restart receiver interrupts, and exit.
 */
  outportb( port->uart_base + IER, IER_RX_DATA );
}
/*
 * In order to close the port, I first disable interrupts
 * at the UART, then disable interrupts from the UART's IRQ
 * line at the 8259 interrupt controller.  DTR, RTS, and
 * OUT2 are all dropped. The UART's previous interrupt
 * handler is restored, and the old break handler
 * is restored.  Finally, the port data structure is freed,
 * and things should be completely back to normal.
 */
void port_close( PORT *port )
{
  unsigned char temp;

  outportb( port->uart_base + IER, 0 );
  temp = (unsigned char) inportb( INT_CONTROLLER + 1 );
  outportb( INT_CONTROLLER + 1, port->irq_mask | temp );
  setvect( port->interrupt_number, port->old_vector );
  setvect( BREAK_VECTOR, old_break_handler );
  outportb( port->uart_base + MCR, 0 );
  free( port );
}
/*
 * This routine is used to send a single character out to
 * the UART.  If there is room in the output buffer, the
 * character is inserted in the buffer.  Then the routine
 * checks to see if Transmit interrupts are presently
 * enabled for this UART.  If they aren't, they are turned
 * on so the ISR will see this new character.
 */
int port_putc( unsigned char c, PORT *port )
{
  if (( port->out.write_index+1 ) == port->out.read_index)
    return( -1 );
  port->out.buffer[ port->out.write_index ] = c;
  port->out.write_index += 1;
  if (( inportb( port->uart_base + IER ) & IER_THRE) == 0 )
    outportb( port->uart_base+IER,IER_THRE | IER_RX_DATA);
  return( c );
}
/*
 * This routine checks to see if there is a character
 * available in the input buffer for the specified port.
 * If there is, it is pulled out and returned to the
 * caller.
 */
int port_getc(PORT *port)
{
  if ( port->in.write_index == port->in.read_index )
    return( -1 );
  else
    return( port->in.buffer[ port->in.read_index++ ] );
}
