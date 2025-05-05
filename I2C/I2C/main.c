
#define F_CPU 1000000UL			//CPU Frequency 1MHz
#define SCL_CLK 1000UL

#define LCD_Dir  DDRB			//Define LCD data port direction
#define LCD_Port PORTB			//Define LCD data port
#define RS PB0				//Define Register Select pin
#define EN PB1 				//Define Enable signal pin
#define RW PB2				//Define Read/Write signal pin

typedef unsigned char byte;
#define BITRATE(TWSR)	((F_CPU/SCL_CLK)-16)/(2*pow(4,(TWSR&((1<<TWPS0)|(1<<TWPS1)))))

#include <avr/io.h>			//Include AVR std. library file
#include <util/delay.h>			// Include Delay header file 
#include <stdio.h>

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); // sending upper nibble 
	LCD_Port &= ~ (1<<RS);		// RS=0, command reg. 
	LCD_Port |= (1<<EN);		// Enable pulse 
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  // sending lower nibble 
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); // sending upper nibble 
	LCD_Port |= (1<<RS);		// RS=1, data reg. 
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (data << 4); // sending lower nibble 
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Init (void)			// LCD Initialize function 
{
	LCD_Dir = 0xFF;			// Make LCD port direction as o/p 
	_delay_ms(20);			// LCD Power ON delay always >15ms 
	
	LCD_Command(0x02);		// send for 4 bit initialization of LCD  
	LCD_Command(0x28);              // 2 line, 5*7 matrix in 4-bit mode 
	LCD_Command(0x0c);              // Display on cursor off
	LCD_Command(0x06);              // Increment cursor (shift cursor to right)
	LCD_Command(0x01);              // Clear display screen
	_delay_ms(2);
}

void LCD_String (char *str)		// Send string to LCD function 
{
	int i;
	for(i=0;str[i]!=0;i++)		// Send each char of string till the NULL 
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)	// Send string to LCD with xy position 
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);	// Command of first row and required position<16 
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);	// Command of first row and required position<16 
	LCD_String(str);		// Call LCD string function 
}

void LCD_Clear()
{
	LCD_Command (0x01);		// Clear display 
	_delay_ms(2);
	LCD_Command (0x80);		// Cursor at home position 
}
 
 void i2c_init(void)
 {
	TWBR = BITRATE(TWSR=0x00);	// Get bit rate register value by formula
	TWCR = 0x04;
 }

 void i2c_start(void)
 {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while(!(TWCR & (1 << TWINT)));
 }

 void i2c_repeated_start(byte add) // I2C repeated start function 
 {
	i2c_start();
 }

 void i2c_write(byte data)
 {
	TWDR=data;			// Copy data in TWI data register 
	TWCR=(1<<TWEN)|(1<<TWINT);	// Enable TWI and clear interrupt flag 
	while(!(TWCR&(1<<TWINT)));	// Wait until TWI finish its current job 
 }

 byte i2c_read_ack()		// I2C read ack function 
 {
	 TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWEA); // Enable TWI, generation of ack 
	 while(!(TWCR&(1<<TWINT)));	// Wait until TWI finish its current job 
	 return TWDR;			// Return received data 
 }

 byte i2c_read_nack()		// I2C read nack function 
 {
	 TWCR=(1<<TWEN)|(1<<TWINT);	// Enable TWI and clear interrupt flag 
	 while(!(TWCR&(1<<TWINT)));	// Wait until TWI finish its current job 
	 return TWDR;		// Return received data 
 }

 void i2c_stop()			// I2C stop function 
 {
	 TWCR=(1<<TWSTO)|(1<<TWINT)|(1<<TWEN); // Enable TWI, generate stop 
	 while(TWCR&(1<<TWSTO));	// Wait until stop condition execution 
 }

 
 void rtc_init(void)
 {
	i2c_init();
	i2c_start();
	i2c_write(0xD0); // address for 1307
	i2c_write(0x07);
	i2c_write(0x00);
	i2c_stop();
 }

 void rtc_setTime(byte h, byte m, byte s)
 {
	i2c_start();
	i2c_write(0xD0);
	i2c_write(0x00);
	i2c_write(s);
	i2c_write(m);
	i2c_write(h);
	i2c_stop();
 }

 void rtc_getTime(byte* h, byte* m, byte* s)
 {
	i2c_start();
	i2c_write(0xD0);
	i2c_write(0x00);
	i2c_stop();
	i2c_start();
	i2c_write(0xD1);
	*s = i2c_read_ack();
	*m = i2c_read_ack();
	*h = i2c_read_nack();
	i2c_stop();
 }

 int bcd_to_decimal(byte x) 
 {
	 return ((x & 0xF0) >> 4) * 10 + (x & 0x0F);
 }

int main()
{
	
	char str[20] = {0};
	byte s = 0;
	byte h = 0;
	byte m = 0;
	LCD_Init();
	LCD_String_xy(0, 0, "Starting");
	rtc_init();
	//rtc_setTime(0x11, 0x12, 0x00); //14:07:00
	
	while(1)
	{
		rtc_getTime(&h, &m, &s);
		sprintf(str, "Time : %02d:%02d:%02d", bcd_to_decimal(h), bcd_to_decimal(m), bcd_to_decimal(s));
		//LCD_Clear();
		LCD_String_xy(0, 0, str);
		_delay_ms(100);
	}
}


