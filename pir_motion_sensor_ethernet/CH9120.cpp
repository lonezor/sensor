#include "CH9120.h"

UCHAR CH9120_Mode = UDP_SERVER; // TCP_SERVER / TCP_CLIENT / UDP_SERVER / UDP_CLIENT
UDOUBLE CH9120_BAUD_RATE = 115200;                // BAUD RATE

UCHAR tx[8] = {0x57, 0xAB};

/******************************************************************************
function:	Send four bytes
parameter:
    data: parameter
    command: command code
Info:  Set mode, enable port, clear serial port, switch DHCP, switch port 2
******************************************************************************/
void CH9120_TX_4_bytes(UCHAR data, int command)
{
    for (int i = 2; i < 4; i++)
    {
        if (i == 2)
            tx[i] = command;
        else
            tx[i] = data;
    }
    DEV_Delay_ms(10);
    for (int o = 0; o < 4; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(10);
    for (int i = 2; i < 4; i++)
        tx[i] = 0;
}

/******************************************************************************
function:	Send five bytes
parameter:
    data: parameter
    command: command code
Info:  Set the local port and target port
******************************************************************************/
void CH9120_TX_5_bytes(UWORD data, int command)
{
    UCHAR Port[2];
    Port[0] = data & 0xff;
    Port[1] = data >> 8;
    for (int i = 2; i < 5; i++)
    {
        if (i == 2)
            tx[i] = command;
        else
            tx[i] = Port[i - 3];
    }
    DEV_Delay_ms(10);
    for (int o = 0; o < 5; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(10);
    for (int i = 2; i < 5; i++)
        tx[i] = 0;
}
/******************************************************************************
function:	Send seven bytes
parameter:
    data: parameter
    command: command code
Info:  Set the IP address, subnet mask, gateway,
******************************************************************************/
void CH9120_TX_7_bytes(UCHAR data[], int command)
{
    for (int i = 2; i < 7; i++)
    {
        if (i == 2)
            tx[i] = command;
        else
            tx[i] = data[i - 3];
    }
    DEV_Delay_ms(10);
    for (int o = 0; o < 7; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(10);
    for (int i = 2; i < 7; i++)
        tx[i] = 0;
}

/******************************************************************************
function:	CH9120_TX_BAUD
parameter:
    data: parameter
    command: command code
Info:  Set baud rate
******************************************************************************/
void CH9120_TX_BAUD(UDOUBLE data, int command)
{
    UCHAR Port[4];
    Port[0] = (data & 0xff);
    Port[1] = (data >> 8) & 0xff;
    Port[2] = (data >> 16) & 0xff;
    Port[3] = data >> 24;

    for (int i = 2; i < 7; i++)
    {
        if (i == 2)
            tx[i] = command;
        else
            tx[i] = Port[i - 3];
    }
    DEV_Delay_ms(10);
    for (int o = 0; o < 7; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(10);
    for (int i = 2; i < 7; i++)
        tx[i] = 0;
}

/******************************************************************************
function:	CH9120_Start
parameter:
Info:  Start configuration Parameters
******************************************************************************/
void CH9120_Start(void)
{
    digitalWrite(CFG_PIN, 0);
    digitalWrite(RES_PIN, 1);
    DEV_Delay_ms(500);
}

/******************************************************************************
function:	CH9120_End
parameter:
Info:  Updating configuration Parameters
******************************************************************************/
void CH9120_End(void)
{
    tx[2] = 0x0d;
    for (int o = 0; o < 3; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(200);
    tx[2] = 0x0e;
    for (int o = 0; o < 3; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(200);
    tx[2] = 0x5e;
    for (int o = 0; o < 3; o++)
        UART_ID1.write(tx[o]);
    DEV_Delay_ms(500);
    gpio_put(CFG_PIN, 1);
}

/******************************************************************************
Function:	CH9120_SetMode
Parameters:
    Mode: Mode parameter
Info:  Configure communication mode
******************************************************************************/
void CH9120_SetMode(UCHAR Mode)
{
    CH9120_TX_4_bytes(Mode, Mode1); //Mode
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetLocalIP
Parameters:
    CH9120_LOCAL_IP: Local IP parameter
Info:  Configure local IP
******************************************************************************/
void CH9120_SetLocalIP(UCHAR CH9120_LOCAL_IP[])
{
    CH9120_TX_7_bytes(CH9120_LOCAL_IP, LOCAL_IP); //LOCALIP
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetSubnetMask
Parameters:
    CH9120_SUBNET_MASK: SUBNET MASK parameter
Info:  Configure subnet mask
******************************************************************************/
void CH9120_SetSubnetMask(UCHAR CH9120_SUBNET_MASK[])
{
    CH9120_TX_7_bytes(CH9120_SUBNET_MASK, SUBNET_MASK); //SUBNET MASK
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetGateway
Parameters:
    CH9120_GATEWAY: Gateway parameter
Info:  Configure gateway
******************************************************************************/
void CH9120_SetGateway(UCHAR CH9120_GATEWAY[])
{
    CH9120_TX_7_bytes(CH9120_GATEWAY, GATEWAY); //GATEWAY
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetTargetIP
Parameters:
    CH9120_TARGET_IP: Target IP parameter
Info:  Configure target IP
******************************************************************************/
void CH9120_SetTargetIP(UCHAR CH9120_TARGET_IP[])
{
    CH9120_TX_7_bytes(CH9120_TARGET_IP, TARGET_IP1); //TARGET IP
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetLocalPort
Parameters:
    CH9120_PORT: Local Port parameter
Info:  Configure local port number
******************************************************************************/
void CH9120_SetLocalPort(UWORD CH9120_PORT)
{
    CH9120_TX_5_bytes(CH9120_PORT, LOCAL_PORT1); //Local port
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetTargetPort
Parameters:
    CH9120_TARGET_PORT: Target Port parameter
Info:  Configure target port number
******************************************************************************/
void CH9120_SetTargetPort(UWORD CH9120_TARGET_PORT)
{
    CH9120_TX_5_bytes(CH9120_TARGET_PORT, TARGET_PORT1); //Target port
    DEV_Delay_ms(100);
}

/******************************************************************************
Function:	CH9120_SetBaudRate
Parameters:
    CH9120_BAUD_RATE: Baud Rate parameter
Info:  Configure communication baud rate
******************************************************************************/
void CH9120_SetBaudRate(UDOUBLE CH9120_BAUD_RATE)
{
    CH9120_TX_BAUD(CH9120_BAUD_RATE, UART1_BAUD1); //Port 1 baud rate
    DEV_Delay_ms(100);
}

/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
    delay(xms);
}

void DEV_Delay_us(UDOUBLE xus)
{
    delayMicroseconds(xus);
}

/******************************************************************************
function:	CH9120_init
parameter:
Info:  Initialize CH9120
******************************************************************************/
void CH9120_init(
  UCHAR local_ip[4],
  UCHAR gateway_ip[4],
  UCHAR subnet_mask[4],
  UCHAR remote_ip[4],
  UWORD local_port,
  UWORD remote_port)
{
    Serial.begin(115200);
    delay(1000);

    UART_ID1.setTX(UART_TX_PIN1);
    UART_ID1.setRX(UART_RX_PIN1);
    UART_ID1.begin(Inti_BAUD_RATE);
    
    pinMode(CFG_PIN,OUTPUT);
    pinMode(RES_PIN,OUTPUT);

    CH9120_Start();
    CH9120_SetMode(CH9120_Mode);
    CH9120_SetLocalIP(local_ip);
    CH9120_SetGateway(gateway_ip);
    CH9120_SetSubnetMask(subnet_mask);
    CH9120_SetTargetIP(remote_ip);
    CH9120_SetLocalPort(local_port);
    CH9120_SetTargetPort(remote_port);
    CH9120_SetBaudRate(CH9120_BAUD_RATE); // Port 1 baud rate
    CH9120_End(); 

    UART_ID1.begin(Transport_BAUD_RATE);
    while (UART_ID1.available())
    {
        UBYTE ch1 = UART_ID1.read();
    }
}

/******************************************************************************
function:	SendUdpPacket
parameter:
Info:  String as UDP payload
******************************************************************************/
bool SendUdpPacket(const char* str)
{
   UART_ID1.write(str);
   return true;
}

/******************************************************************************
function:	RecvUdpPacket
parameter:
Info:  String as UDP payload
******************************************************************************/
bool RecvUdpPacket(char* str, int str_len)
{
  if (!UART_ID1.available()) {
      return false;
  }
    
  static uint8_t buffer[1400];
  memset(buffer, 0, sizeof(buffer));
    
  int rx_bytes = UART_ID1.readBytesUntil('\n', buffer, sizeof(buffer)-1);

  // This API does not show the divide between physical packets. The
  // protocol is sending new packets with updated states at regular
  // intervals. Purge the pending bytes since this function
  // only delivers a single packet at a time
  while (UART_ID1.available()) {
      UART_ID1.read();
  }

  snprintf(str, str_len, "%s", buffer);

  return true;
}

