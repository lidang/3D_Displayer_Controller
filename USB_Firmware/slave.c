#pragma NOIV                    // Do not generate interrupt vectors
//-----------------------------------------------------------------------------
//   File:      slave.c
//   Contents:  Hooks required to implement USB peripheral function.
//              Code written for FX2 REVE 56-pin and above.
//              This firmware is used to demonstrate FX2 Slave FIF
//              operation.
//   Copyright (c) 2003 Cypress Semiconductor All rights reserved
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

#define LED_ALL         (bmBIT0 | bmBIT1 | bmBIT2 | bmBIT3)

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings

// EZUSB FX2 PORTA = slave fifo enable(s), when IFCFG[1:0]=11
//sbit PA0 = IOA ^ 0;             // alt. func., INT0#
//sbit PA1 = IOA ^ 1;             // alt. func., INT1#
// sbit PA2 = IOA ^ 2;          // is SLOE
//sbit PA3 = IOA ^ 3;             // alt. func., WU2
// sbit PA4 = IOA ^ 4;          // is FIFOADR0
// sbit PA5 = IOA ^ 5;          // is FIFOADR1
// sbit PA6 = IOA ^ 6;          // is PKTEND
// sbit PA7 = IOA ^ 7;          // is FLAGD

// EZUSB FX2 PORTC i/o...       port NA for 56-pin FX2
// sbit PC0 = IOC ^ 0;
// sbit PC1 = IOC ^ 1;
// sbit PC2 = IOC ^ 2;
// sbit PC3 = IOC ^ 3;
// sbit PC4 = IOC ^ 4;
// sbit PC5 = IOC ^ 5;
// sbit PC6 = IOC ^ 6;
// sbit PC7 = IOC ^ 7;

// EZUSB FX2 PORTB = FD[7:0], when IFCFG[1:0]=11
// sbit PB0 = IOB ^ 0;
// sbit PB1 = IOB ^ 1;
// sbit PB2 = IOB ^ 2;
// sbit PB3 = IOB ^ 3;
// sbit PB4 = IOB ^ 4;
// sbit PB5 = IOB ^ 5;
// sbit PB6 = IOB ^ 6;
// sbit PB7 = IOB ^ 7;

// EZUSB FX2 PORTD = FD[15:8], when IFCFG[1:0]=11 and WORDWIDE=1
//sbit PD0 = IOD ^ 0;
//sbit PD1 = IOD ^ 1;
//sbit PD2 = IOD ^ 2;
//sbit PD3 = IOD ^ 3;
//sbit PD4 = IOD ^ 4;
//sbit PD5 = IOD ^ 5;
//sbit PD6 = IOD ^ 6;
//sbit PD7 = IOD ^ 7;

// EZUSB FX2 PORTE is not bit-addressable...

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
// The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------
//void LED_Off (BYTE LED_Mask);
//void LED_On (BYTE LED_Mask);

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------
void TD_Init( void )
{ // Called once at startup


  CPUCS = 0x10; // CLKSPD[1:0]=10, for 48MHz operation, output CLKOUT

  PINFLAGSAB = 0x4A;			// FLAGB - EP2PF ,FLAGA - EP6EF
  SYNCDELAY;
  PINFLAGSCD = 0x08;			// FLAGC - EP2EF
  SYNCDELAY;
  PORTACFG |= 0x80;
  SYNCDELAY;
  IFCONFIG = 0x03; // for async? for sync?
  SYNCDELAY;
  FIFOPINPOLAR |= 0x3F;
  SYNCDELAY;

  CPUCS |= 0x02;
	SYNCDELAY;  
	
	REVCTL = 0x01;
	SYNCDELAY;  

  // IFCLKSRC=1   , FIFOs executes on internal clk source 
  // xMHz=1       , 48MHz operation
  // IFCLKOE=1    ,Drive IFCLK pin signal at 48MHz
  // IFCLKPOL=0   , Don't invert IFCLK pin signal from internal clk
  // ASYNC=0      , master samples synchronous
  // GSTATE=0     , Don't drive GPIF states out on PORTE[2:0], debug WF
  // IFCFG[1:0]=11, FX2 in slave FIFO mode


  // Registers which require a synchronization delay, see section 15.14
  // FIFORESET        FIFOPINPOLAR
  // INPKTEND         OUTPKTEND
  // EPxBCH:L         REVCTL
  // GPIFTCB3         GPIFTCB2
  // GPIFTCB1         GPIFTCB0
  // EPxFIFOPFH:L     EPxAUTOINLENH:L
  // EPxFIFOCFG       EPxGPIFFLGSEL
  // PINFLAGSxx       EPxFIFOIRQ
  // EPxFIFOIE        GPIFIRQ
  // GPIFIE           GPIFADRH:L
  // UDMACRCH:L       EPxGPIFTRIG
  // GPIFTRIG
  
  // Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
  //      ...these have been replaced by GPIFTC[B3:B0] registers


  
 

  // EP4 and EP8 are not used in this implementation...
       
	EP4CFG = 0x01;                //clear valid bit
  SYNCDELAY;                     
  EP8CFG = 0x01;                //clear valid bit
  SYNCDELAY;   
  EP2CFG = 0xA0;                //out 512 bytes, 4x, bulk
  SYNCDELAY;                    
  EP6CFG = 0xE0;                // in 512 bytes, 4x, bulk
  SYNCDELAY;              

  FIFORESET = 0x80;             // activate NAK-ALL to avoid race conditions
  SYNCDELAY;                    // see TRM section 15.14
  FIFORESET = 0x02;             // reset, FIFO 2
  SYNCDELAY;                    // 
  FIFORESET = 0x04;             // reset, FIFO 4
  SYNCDELAY;                    // 
  FIFORESET = 0x06;             // reset, FIFO 6
  SYNCDELAY;                    // 
  FIFORESET = 0x08;             // reset, FIFO 8
  SYNCDELAY;                    // 
  FIFORESET = 0x00;             // deactivate NAK-ALL
  SYNCDELAY;   
	
	EP2FIFOCFG = 0x00;            
  SYNCDELAY;
  EP2FIFOCFG = 0x10;            // AUTOOUT=1, WORDWIDE=0
  SYNCDELAY;
	
	EP6FIFOCFG = 0x00;            
  SYNCDELAY;
  EP6FIFOCFG = 0x08;            // AUTOIN=1, ZEROLENIN=1, WORDWIDE=0
  SYNCDELAY;
	
	EP6AUTOINLENH = 0x02;
	SYNCDELAY;
	EP6AUTOINLENL = 0x00;
	SYNCDELAY;
	EP2FIFOPFH = 0x88;
	SYNCDELAY;
	EP2FIFOPFL = 0x00;
	SYNCDELAY;
	
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;

}

void TD_Poll( void )
{ // Called repeatedly while the device is idle

}

BOOL TD_Suspend( void )          
{ // Called before the device goes into suspend mode
   return( TRUE );
}

BOOL TD_Resume( void )          
{ // Called after the device resumes
   return( TRUE );
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor( void )
{
   return( TRUE );
}

BOOL DR_SetConfiguration( void )   
{ // Called when a Set Configuration command is received
  
  if( EZUSB_HIGHSPEED( ) )
  { // ...FX2 in high speed mode
    EP6AUTOINLENH = 0x02;
    SYNCDELAY;
    EP8AUTOINLENH = 0x02;   // set core AUTO commit len = 512 bytes
    SYNCDELAY;
    EP6AUTOINLENL = 0x00;
    SYNCDELAY;
    EP8AUTOINLENL = 0x00;
  }
  else
  { // ...FX2 in full speed mode
    EP6AUTOINLENH = 0x00;
    SYNCDELAY;
    EP8AUTOINLENH = 0x00;   // set core AUTO commit len = 64 bytes
    SYNCDELAY;
    EP6AUTOINLENL = 0x40;
    SYNCDELAY;
    EP8AUTOINLENL = 0x40;
  }
      
  Configuration = SETUPDAT[ 2 ];
  return( TRUE );        // Handled by user code
}

BOOL DR_GetConfiguration( void )   
{ // Called when a Get Configuration command is received
   EP0BUF[ 0 ] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);          // Handled by user code
}

BOOL DR_SetInterface( void )       
{ // Called when a Set Interface command is received
   AlternateSetting = SETUPDAT[ 2 ];
   return( TRUE );        // Handled by user code
}

BOOL DR_GetInterface( void )       
{ // Called when a Set Interface command is received
   EP0BUF[ 0 ] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return( TRUE );        // Handled by user code
}

BOOL DR_GetStatus( void )
{
   return( TRUE );
}

BOOL DR_ClearFeature( void )
{
   return( TRUE );
}

BOOL DR_SetFeature( void )
{
   return( TRUE );
}

BOOL DR_VendorCmnd( void )
{
  return( TRUE );
}

BOOL RstAll(void)
{
	FIFORESET = 0x80; 
	SYNCDELAY;
	FIFORESET = 0X02;
	SYNCDELAY;
	FIFORESET = 0x06;
	SYNCDELAY;
	FIFORESET = 0x00;                
	SYNCDELAY;   
	EP2FIFOCFG = 0x00;            
	SYNCDELAY;
	EP2FIFOCFG = 0x10;  //8bits          
	SYNCDELAY;
	EP6FIFOCFG = 0x00;            
	SYNCDELAY;
	EP6FIFOCFG = 0x08;  //8bits               
	SYNCDELAY;
	EP6AUTOINLENH = 0x02;
	SYNCDELAY;
	EP6AUTOINLENL = 0x00;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
	INPKTEND = 0x84;
	SYNCDELAY;
		
	return( TRUE );
}	

BOOL GetState(void)
{
	EP0BUF[0]=EP2CS;
	EP0BUF[1]=EP2FIFOFLGS;
	EP0BUF[2]=EP2CFG;
	EP0BUF[3]=EP2FIFOCFG;
	EP0BUF[4]=EP2FIFOBCH;
	EP0BUF[5]=EP2FIFOBCL;
	EP0BUF[6]=EP2BCH;
	EP0BUF[7]=EP2BCL;
	
	EP0BUF[8]=EP6CS;
	EP0BUF[9]=EP2FIFOFLGS;
	EP0BUF[10]=EP6CFG;
	EP0BUF[11]=EP6FIFOCFG;
	EP0BUF[12]=EP6FIFOBCH;
	EP0BUF[13]=EP6FIFOBCL;
	EP0BUF[14]=EP6BCH;
	EP0BUF[15]=EP6BCL;
	
	EP0BUF[16]=EP2FIFOBUF[0];
	EP0BUF[17]=EP2FIFOBUF[1];
	EP0BUF[18]=EP6FIFOBUF[0];
	EP0BUF[19]=EP6FIFOBUF[1];
	EP0BUF[20]=EP6FIFOBUF[1022];
	EP0BUF[21]=EP6FIFOBUF[1023];
	EP0BUF[22]=EP6FIFOBUF[1024];
	EP0BUF[23]=EP6FIFOBUF[1025];
	EP0BUF[24]=EP6FIFOBUF[2046];
	EP0BUF[25]=EP6FIFOBUF[2047];
	EP0BCH=0x00;
	EP0BCL=0x1A;
	
	return( TRUE );
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav( void ) interrupt 0
{
   GotSUD = TRUE;         // Set flag
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUDAV;      // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUTOK;      // Clear SUTOK IRQ
}

void ISR_Sof( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSOF;        // Clear SOF IRQ
}

void ISR_Ures( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmURES;       // Clear URES IRQ
}

void ISR_Susp( void ) interrupt 0
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUSP;
}

void ISR_Highspeed( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }

   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack( void ) interrupt 0
{
}
void ISR_Stub( void ) interrupt 0
{
}
void ISR_Ep0in( void ) interrupt 0
{
}
void ISR_Ep0out( void ) interrupt 0
{
}
void ISR_Ep1in( void ) interrupt 0
{
}
void ISR_Ep1out( void ) interrupt 0
{
}
void ISR_Ep2inout( void ) interrupt 0
{
}
void ISR_Ep4inout( void ) interrupt 0
{
}
void ISR_Ep6inout( void ) interrupt 0
{
}
void ISR_Ep8inout( void ) interrupt 0
{
}
void ISR_Ibn( void ) interrupt 0
{
}
void ISR_Ep0pingnak( void ) interrupt 0
{
}
void ISR_Ep1pingnak( void ) interrupt 0
{
}
void ISR_Ep2pingnak( void ) interrupt 0
{
}
void ISR_Ep4pingnak( void ) interrupt 0
{
}
void ISR_Ep6pingnak( void ) interrupt 0
{
}
void ISR_Ep8pingnak( void ) interrupt 0
{
}
void ISR_Errorlimit( void ) interrupt 0
{
}
void ISR_Ep2piderror( void ) interrupt 0
{
}
void ISR_Ep4piderror( void ) interrupt 0
{
}
void ISR_Ep6piderror( void ) interrupt 0
{
}
void ISR_Ep8piderror( void ) interrupt 0
{
}
void ISR_Ep2pflag( void ) interrupt 0
{
}
void ISR_Ep4pflag( void ) interrupt 0
{
}
void ISR_Ep6pflag( void ) interrupt 0
{
}
void ISR_Ep8pflag( void ) interrupt 0
{
}
void ISR_Ep2eflag( void ) interrupt 0
{
}
void ISR_Ep4eflag( void ) interrupt 0
{
}
void ISR_Ep6eflag( void ) interrupt 0
{
}
void ISR_Ep8eflag( void ) interrupt 0
{
}
void ISR_Ep2fflag( void ) interrupt 0
{
}
void ISR_Ep4fflag( void ) interrupt 0
{
}
void ISR_Ep6fflag( void ) interrupt 0
{
}
void ISR_Ep8fflag( void ) interrupt 0
{
}
void ISR_GpifComplete( void ) interrupt 0
{
}
void ISR_GpifWaveform( void ) interrupt 0
{
}
