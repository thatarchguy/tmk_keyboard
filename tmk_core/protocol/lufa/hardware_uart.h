#ifndef HARDWARE_UART
#define HARDWARE_UART

/* host role */
void uart_transmit( unsigned char data );
unsigned char uart_receive(void);
void uart_init(void);
int uart_dataAvailable(void);
void uart_print( char data[] );
#endif