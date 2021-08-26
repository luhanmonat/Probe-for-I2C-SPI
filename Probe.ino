/*
 * Send and recieve bytes via SPI or I2C.
 * 3/2021 Luhan Monat
 * Coded for Arduino Nano
 */

//  define port and bits used on nano
//  change these for other boards

/*  Commands    (upper or lower case)
 *  ------------------------------------------  
 *  xxyy zz     any two hex digits (spaces optional)
 *  .ddd        Read ddd (decimal) bytes as hex (end with space or end of line)
 *  /           Create START condition (select)
 *  ;           Create STOP (automatic at end of line)
 *  R           Reverse bit sense
 *  M           Mode SPI, I2C 
 *  Wnnn        Wait nnn milliseconds
 *  *ddd        Repeat last command byte sent ddd times
 */

#define PORT      PORTD
#define PORTIN    PIND
#define PORT_DIR  DDRD

#define START 2
#define STOP  3

//  SPI Mode
#define SCK  4     // SPI CLOCK
#define MOSI 5      // MASTER OUT
#define MISO 3      // MASTER IN
#define NSEL 2      // *SELECT
#define CEN  6      // nrf chip on

//  I2C Mode
#define SCL   2       // I2C clock
#define SDA   3       // I2C data
#define IACK  1

#define SPIMODE 1
#define I2CMODE 2

int mode,reverse,space,index;
char ErrorFlag;
char buff[40];
byte  last,endline;



void setup() {
  
  Serial.begin(115200);
  PORT_DIR = 0b00110100;    // only bit 4 is input
  space=20;
  mode=SPIMODE;
  DoSel(STOP);
}

void loop() {
  int i,j,dat,cmd;

  Serial.println("Version=4");
  
  rerun:
  endline=0;
  ErrorFlag=0;
  DoSel(STOP);
  switch (mode) {
   case SPIMODE: Serial.print("\nSPI"); break;
   case I2CMODE: Serial.print("\nI2C"); break;
  }
  if(reverse) 
    Serial.print("/r");
  Serial.print(">");
  while(Serial.available()) 
  Serial.read();            // clear out input buffer
 
  next:
  cmd=toupper(GetSerial());
  if(endline) cmd='\r';
  Serial.write(cmd);
  switch (cmd) {
    case ' ':
    case ',': break;
    case '/': DoSel(START); break;
    case ';': DoSel(STOP); delay(1); break;
    case '\r':
    case '\n': Serial.print(" ok"); goto rerun;
    case 'M': NewMode(0); break;
    case 'R': reverse^=1; break;
    case 'S': ScanI2C(); break;
    case '.': RecvHex(); goto rerun;
    case 'W': delay(GetDecimal()); break;
    case '*': Repeat(); break;
    default:  SendHex(cmd);
              if(ErrorFlag) {
                Serial.write(ErrorFlag);    // show bad character
                Serial.print("?");
                delay(400);
                goto rerun;                 // abort the rest
              }
  }
  goto next;

}


void NewMode(int init) {

  if(init)
    mode=init;      // set initial mode SPI/I2C
  else
    if(++mode>2)
      mode=1;
  DoSel(STOP);
}

void DoSel(int func) {

  switch(mode) {
    case SPIMODE: bitClear(PORT_DIR,MISO);    // MISO is input
                  bitSet(PORT_DIR,MOSI);      // MOSI to output
                  if(func==START) {
                    bitClear(PORT,NSEL); Dx();
                  }
                  if(func==STOP) {
                    bitSet(PORT,NSEL); Dx(); Dx();
                  }
                  break;
    case I2CMODE: if(func==START) {
                //    bitClear(PORT,SCL); Dx();
                    bitSet(PORT,SDA); Dx();
                    bitSet(PORT,SCL); Dx();
                    bitClear(PORT,SDA); Dx(); // start condition
                    bitClear(PORT,SCL); Dx();
                  }
                  if(func==STOP) {
                    bitSet(PORT,SCL); Dx();
                    bitSet(PORT,SDA); Dx(); Dx(); // stop condition
                  }       
                  break;
  }
}

//  Scan the entire I2C address space

void  ScanI2C() {
  int i,ack;

  Serial.println();
  Serial.print("Scanning - ");
  for(i=0;i<256;i++) {
    delay(10);            // delay for dramatic effect
    DoSel(START);         // create start condition
    ack=I2CWbyte(i);      // anybody home?
    DoSel(STOP);          // complete operation
    if(!ack) {            // zero = ACK true
      Serial.print(i,HEX);
      Serial.print(' ');      
    }
  }
}

void RecvHex() {
  int loops,i,data;
  
  loops=GetDecimal();
  Serial.print(loops);
  for(i=0;i<loops;i++) {
    if(i%32==0)
      Serial.println();
    if(i==(loops-1))
      data=SwitchIn(1);     // no ack on last byte
    else
      data=SwitchIn(0);     // normal ack on each byte
    Printx(" %02X",data);
  }
}

// Convert monitor ascii to decimal value

word  GetDecimal() {
  word result;
  char digit;

  result=0;
  while(1) {
    digit=toupper(GetSerial());
    if(digit=='\r')
      endline=1;
    if((digit>='0')&&(digit<='9'))
      result=result*10+(digit-'0');
    else
      return(result);
  }   
}

void  Repeat() {
int i,count;

  count=GetDecimal()-1;       // one already sent
  for(i=0;i<count;i++)
    SwitchOut(last);
  
}

void SendHex(char cc) {
int dat,low;
  
  dat=Nibble(cc)<<4;
  low=toupper(GetSerial());
  Serial.write(low);
  dat+=Nibble(low);
  if(!ErrorFlag)
    SwitchOut(dat);
  last=dat;               // save for repeats
}


byte Nibble(char x) {

  x=toupper(x);
  if((x>='0')&&(x<='9'))
    return(x-'0');
  if((x>='A')&&(x<='F'))
    return(10+(x-'A'));
  ErrorFlag=x;
  return(0);
}



void SwitchOut(byte data) {
byte rtn;

  switch (mode) {
    case SPIMODE: rtn=SPI(data);
                  Serial.print(':');
                  Serial.print(rtn,HEX);
                  Serial.print("!");
                  break;
    case I2CMODE: rtn=I2CWbyte(data);
                  if(rtn==0) Serial.print("!");
                  else    Serial.print("?");
                  break;
  }
  
}



byte  SwitchIn(byte noack) {
byte  data;

  switch (mode) {
    case SPIMODE: data=SPI(0xFF); break;
    case I2CMODE: data=I2CRbyte(noack); break;
  }
  return(data);
}



int GetSerial() {

  while(!Serial.available())
    ;
  return(Serial.read());
}

int nrf(int cmd, int dat) {

  bitClear(PORT,NSEL);   // Chip select
  SPI(cmd);
  SPI(dat);
  bitSet(PORT,NSEL);
 
}

// Send & Recieve SPI data via bit-banging
// 80k bytes/sec w/16 mhz clock

byte  SPI(int data) {
  int i;
  byte rdata;

  rdata=0;     // clear return data

for(i=0x80;i>0;i>>=1) {       // high order bit 1st
   bitClear(PORT,SCK);
   Dx();
   if(data&i)
     bitSet(PORT,MOSI);
   else
     bitClear(PORT,MOSI);
   rdata<<=1;                 // high order 1st
   Dx();
   bitSet(PORT,SCK);          // rising edge clocks data
   Dx();
   if(bitRead(PORTIN,MISO))     // get return data on bit 4
     rdata|=1;
   Dx();
}  
  bitClear(PORT,SCK);
  bitClear(PORT,MOSI);
  return(rdata);    
} 


byte  I2CWbyte(byte dat) {
volatile byte  i,x,mask;

  mask=0x80;
  for(i=0;i<8;i++) {
    Dx();
    if(dat&mask)
      bitSet(PORT,SDA);
    else
      bitClear(PORT,SDA);
    Dx();
    bitSet(PORT,SCL); Dx();
    mask>>=1;
    bitClear(PORT,SCL);
  }
  Dx();
  bitClear(PORT_DIR,SDA);    // Set for input
  bitSet(PORT,SCL); Dx();
  x=bitRead(PORTIN,SDA); Dx();   // Read ACK bit
  bitClear(PORT,SCL);
  bitSet(PORT_DIR,SDA); Dx();
  bitClear(PORT,SDA);
  return(x);
}


// read byte with ACK unless nack.

byte  I2CRbyte(byte nack) {
byte  i,x,dat;

  bitClear(PORT_DIR,SDA);
  dat=0;
  for(i=0;i<8;i++) {
    bitSet(PORT,SCL); Dx();
    x=bitRead(PORTIN,SDA); 
    bitClear(PORT,SCL); Dx();
    dat<<=1;
    bitWrite(dat,0,x);
  }
  bitSet(PORT_DIR,SDA);
  if(nack)
    bitSet(PORT,SDA);
  else
    bitClear(PORT,SDA);
  Dx();
  bitSet(PORT,SCL); Dx();
  bitClear(PORT,SCL); Dx();
  bitClear(PORT,SDA); Dx();
  return(dat);
}


// This delay help make data easier to see on scope.

void Dx() {

  delayMicroseconds(10);

}


void  Printx(char *fmt, int dat) {
char  ary[40];

  sprintf(ary,fmt,dat);
  Serial.print(ary);
}
