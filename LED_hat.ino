#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

/*Pin definitions*/
#define SoftUartRx   10
#define SoftUartTx   11
#define BLstatus     8
#define LEDPin       13
#define NeoPixelPin  5

/*Pixel count*/
#define PixelCount  238

/*Matrix x,y width, height*/
#define PixelWidth  34
#define PixelHeight 7

/*Frames per 4 seconds*/
uint8_t fp4s = 40;

/*OperationModes*/
/*0=menu ; 1=testmode*/
uint8_t mode = 1;
uint8_t lastmode = 1;
uint8_t menue_input_interface; /* 0=bluetooth 1= serial */

/*Text Ticker Message*/
signed char message[41]; //Byte[40] is reserved for length

/*color modes for Text Ticker*/
uint8_t colormode = 0;
uint32_t color;            //mode = 0;
uint32_t multi_color[40];  //mode = 1, 2


/*Font definition */
PROGMEM const uint8_t letterA[] = { 3, 0, 1, 6,  2, 0, 1, 3,  2, 3, 1, 3,  3, 4, 1, 6,  0};
PROGMEM const uint8_t letterB[] = { 3, 0, 0, 6,  2, 0, 0, 3,  2, 3, 1, 3,  2, 6, 0, 3,  3, 4, 1, 2,  3, 4, 4, 5,  0};
PROGMEM const uint8_t letterC[] = { 3, 0, 1, 5,  2, 0, 1, 3,  2, 6, 1, 3,  1, 4, 1,     1, 4, 5,     0};
PROGMEM const uint8_t letterD[] = { 3, 0, 0, 6,  2, 0, 1, 3,  2, 6, 1, 3,  3, 4, 1, 5,  0};
PROGMEM const uint8_t letterE[] = { 3, 0, 0, 6,  2, 0, 1, 4,  2, 3, 1, 3,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterF[] = { 3, 0, 0, 6,  2, 0, 1, 4,  2, 3, 1, 3,  0};
PROGMEM const uint8_t letterG[] = { 3, 0, 1, 5,  2, 0, 1, 3,  2, 4, 2, 4,  2, 6, 1, 4,  3, 4, 4, 6,  0};
PROGMEM const uint8_t letterH[] = { 3, 0, 0, 6,  2, 3, 1, 3,  3, 4, 0, 6,  0};
PROGMEM const uint8_t letterI[] = { 2, 0, 1, 3,  3, 2, 1, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterJ[] = { 1, 0, 5,     2, 6, 1, 3,  3, 4, 0, 5,  0};
PROGMEM const uint8_t letterK[] = { 3, 0, 0, 6,  1, 4, 0,     1, 3, 1,     1, 2, 2,     1, 1, 3,     1, 2, 4,     1, 3, 5,     1, 4, 6,     0};
PROGMEM const uint8_t letterL[] = { 3, 0, 0, 6,  2, 6, 1, 4,  0};
PROGMEM const uint8_t letterM[] = { 3, 0, 0, 6,  1, 1, 1,     3, 2, 2, 3,  1, 3, 1,     3, 4, 0, 6,  0};
PROGMEM const uint8_t letterN[] = { 3, 0, 0, 6,  1, 1, 2,     1, 2, 3,     1, 3, 4,     3, 4, 0, 6,  0};
PROGMEM const uint8_t letterO[] = { 3, 0, 1, 5,  2, 0, 1, 3,  2, 6, 1, 3,  3, 4, 1, 5,  0};
PROGMEM const uint8_t letterP[] = { 3, 0, 0, 6,  2, 0, 1, 3,  2, 3, 1, 3,  3, 4, 1, 2,  0};
PROGMEM const uint8_t letterQ[] = { 3, 0, 1, 5,  2, 0, 1, 3,  2, 6, 1, 2,  3, 4, 1, 4,  1, 2, 4,     1, 3, 5,     1, 4, 6,     0};
PROGMEM const uint8_t letterR[] = { 3, 0, 0, 6,  2, 0, 1, 3,  2, 3, 1, 3,  3, 4, 1, 2,  1, 2, 4,     1, 3, 5,     1, 4, 6,     0};
PROGMEM const uint8_t letterS[] = { 3, 0, 1, 2,  2, 0, 1, 3,  1, 4, 1,     2, 3, 1, 3,  3, 4, 4, 5,  2, 6, 1, 3,  1, 0, 5,     0};
PROGMEM const uint8_t letterT[] = { 2, 0, 0, 4,  3, 2, 1, 6,  0};
PROGMEM const uint8_t letterU[] = { 3, 0, 0, 5,  2, 6, 1, 3,  3, 4, 0, 5,  0};
PROGMEM const uint8_t letterV[] = { 3, 0, 0, 2,  3, 1, 3, 4,  3, 2, 5, 6,  3, 3, 3, 4,  3, 4, 0, 3,  0};
PROGMEM const uint8_t letterW[] = { 3, 0, 0, 6,  3, 4, 0, 6,  3, 2, 3, 4,  1, 1, 5,     1, 3, 5,     0};
PROGMEM const uint8_t letterX[] = { 3, 0, 0, 1,  3, 4, 0, 1,  1, 1, 2,     1, 2, 3,     1, 3, 2,     1, 1, 4,     1, 3, 4,  3, 0, 5, 6,  3, 4, 5, 6,  0};
PROGMEM const uint8_t letterY[] = { 3, 0, 0, 1,  3, 4, 0, 1,  1, 1, 2,     1, 3, 2,     3, 2, 3, 6,  0};
PROGMEM const uint8_t letterZ[] = { 2, 0, 0, 4,  3, 4, 0, 1,  2, 6, 0, 4,  3, 0, 5, 6,  1, 1, 4,     1, 2, 3,     1, 3, 2,  0};

PROGMEM const uint8_t lettera[] = { 2, 2, 1, 2,  3, 3, 3, 5,  1, 0, 5,     2, 4, 1, 3,  2, 6, 1, 4,  0};
PROGMEM const uint8_t letterb[] = { 3, 0, 0, 6,  1, 1, 3,     2, 2, 2, 3,  3, 4, 3, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterc[] = { 3, 0, 3, 5,  2, 2, 1, 3,  2, 6, 1, 3,  1, 4, 5,     0};
PROGMEM const uint8_t letterd[] = { 3, 4, 0, 6,  2, 2, 1, 2,  3, 0, 3, 5,  1, 3, 3,     2, 6, 1, 3,  0};
PROGMEM const uint8_t lettere[] = { 2, 2, 1, 3,  3, 0, 3, 5,  2, 4, 1, 3,  1, 4, 3,     2, 6, 1, 4,  0};
PROGMEM const uint8_t letterf[] = { 3, 1, 1, 6,  2, 3, 0, 2,  1, 2, 0,     1, 3, 1,     0};
PROGMEM const uint8_t letterg[] = { 3, 0, 1, 3,  2, 0, 1, 3,  3, 4, 1, 5,  2, 4, 1, 3,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterh[] = { 3, 0, 0, 6,  1, 1, 3,     2, 2, 2, 3,  3, 4, 3, 6,  0};
PROGMEM const uint8_t letteri[] = { 1, 2, 0,     2, 2, 1, 2,  3, 2, 3, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterj[] = { 1, 3, 0,     2, 2, 2, 3,  3, 3, 3, 5,  2, 6, 1, 2,  3, 0, 4, 5,  0};
PROGMEM const uint8_t letterk[] = { 3, 0, 0, 6,  1, 1, 4,     1, 2, 3,     1, 3, 2,     1, 2, 5,     1, 3, 6,  0};
PROGMEM const uint8_t letterl[] = { 2, 0, 1, 2,  3, 2, 0, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterm[] = { 3, 0, 3, 6,  1, 1, 2,     3, 2, 3, 4,  1, 3, 2,     3, 4, 3, 6,  0};
PROGMEM const uint8_t lettern[] = { 3, 0, 2, 6,  1, 1, 3,     2, 2, 2, 3,  3, 4, 3, 6,  0};
PROGMEM const uint8_t lettero[] = { 3, 0, 3, 5,  3, 4, 3, 5,  2, 2, 1, 3,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letterp[] = { 3, 0, 2, 6,  3, 4, 3, 4,  2, 2, 1, 3,  2, 5, 2, 3,  1, 1, 4,     0};
PROGMEM const uint8_t letterq[] = { 3, 0, 3, 4,  3, 3, 2, 6,  1, 1, 5,     1, 2, 4,     1, 4, 6,     2, 2, 1, 2, 0};
PROGMEM const uint8_t letterr[] = { 3, 1, 2, 6,  1, 2, 3,     2, 2, 3, 4,  0};
PROGMEM const uint8_t letters[] = { 1, 0, 3,     2, 2, 1, 4,  2, 4, 1, 3,  2, 6, 0, 3,  1, 4, 5,     0};
PROGMEM const uint8_t lettert[] = { 3, 1, 1, 5,  1, 0, 2,     1, 2, 2,     1, 2, 6,     1, 3, 5,     0};
PROGMEM const uint8_t letteru[] = { 3, 0, 2, 5,  3, 4, 2, 6,  2, 6, 1, 2,  1, 3, 5,     0};
PROGMEM const uint8_t letterv[] = { 3, 0, 2, 4,  3, 4, 2, 4,  1, 1, 5,     1, 3, 5,     1, 2, 6,     0};
PROGMEM const uint8_t letterw[] = { 3, 0, 2, 5,  3, 4, 2, 5,  3, 2, 4, 5,  1, 1, 6,     1, 3, 6,     0};
PROGMEM const uint8_t letterx[] = { 1, 0, 2,     1, 1, 3,     1, 2, 4,     1, 3, 5,     1, 4, 6,     1, 4, 2,     1, 3, 3,     1, 1, 5,     1, 0, 6,     0};
PROGMEM const uint8_t lettery[] = { 1, 0, 2,     1, 1, 3,     1, 3, 3,     1, 4, 2,     3, 2, 4, 6,  0};
PROGMEM const uint8_t letterz[] = { 2, 2, 0, 4,  2, 6, 0, 4,  1, 1, 5,     1, 2, 4,     1, 3, 3,     0};

PROGMEM const uint8_t letter0[] = { 3, 0, 1, 5,  2, 0, 1, 3,  2, 6, 1, 3,  3, 4, 1, 5,  1, 1, 4,     1, 2, 3,     1, 3, 2,     0};
PROGMEM const uint8_t letter1[] = { 3, 2, 0, 6,  1, 1, 1,     2, 6, 1, 3,  0};
PROGMEM const uint8_t letter2[] = { 2, 6, 0 ,4,  1, 0, 5,     1, 1, 4,     2, 3, 2, 3,  3, 4, 1, 2,  2, 0, 1, 3,  1, 0, 1,     0};
PROGMEM const uint8_t letter3[] = { 1, 0, 1,     2, 0, 1, 3,  3, 4, 1, 2,  2, 3, 1, 3,  3, 4, 4, 5,  2, 6, 1, 3,  1, 0, 5,     0};
PROGMEM const uint8_t letter4[] = { 3, 3, 0, 6,  2, 4, 0, 4,  1, 0, 3,     1, 1, 2,     1, 2, 1,     0};
PROGMEM const uint8_t letter5[] = { 2, 0, 0, 4,  3, 0, 1, 2,  2, 2, 1, 3,  3, 4, 3, 5,  2, 6, 1, 3,  1, 0, 5,     0};
PROGMEM const uint8_t letter6[] = { 3, 0, 2, 5,  2, 0, 2, 3,  1, 1, 1,     2, 3, 1, 3,  3, 4, 4, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letter7[] = { 2, 0, 1, 4,  1, 4, 1,     1, 3, 2,     1, 2, 3,     3, 1, 4, 6,  0};
PROGMEM const uint8_t letter8[] = { 2, 0, 1, 3,  3, 0, 1, 2,  3, 4, 1, 2,  2, 3, 1, 3,  3, 0, 4, 5,  3, 4, 4, 5,  2, 6, 1, 3,  0};
PROGMEM const uint8_t letter9[] = { 2, 0, 1, 3,  3, 0, 1, 2,  3, 4, 1, 4,  2, 3, 1, 4,  1, 3, 5,     2, 6, 1, 2,  0};
PROGMEM const uint8_t specia0[] = { 3, 2, 0, 4,  1, 2, 6,     0};
PROGMEM const uint8_t specia1[] = { 3, 1, 0, 1,  3, 3, 0, 1,  0};
PROGMEM const uint8_t specia2[] = { 3, 1, 0, 6,  3, 3, 0, 6,  1, 0, 2,     1, 0, 4,     1, 2, 2,     1, 2, 4,     1, 4, 2,     1, 4, 4,     0};
PROGMEM const uint8_t specia3[] = { 3, 2, 0, 6,  2, 1, 1, 4,  2, 5, 1, 4,  1, 0, 2,     1, 1, 3,     1, 3, 3,     1, 4, 4,     0};
PROGMEM const uint8_t specia4[] = { 2, 0, 0, 1,  2, 1, 0, 1,  2, 5, 3, 4,  2, 6, 3, 4,  1, 0, 5,     1, 1, 4,     1, 2, 3,     1, 3, 2,     1, 4, 1,     0};
PROGMEM const uint8_t specia5[] = { 1, 4, 4,     1, 3, 5,     2, 6, 1, 2,  3, 0, 4, 5,  1, 1, 3,     3, 2, 1, 2,  1, 1, 0,     3, 0, 1, 2,  1, 2, 4,     1, 4, 6,     0};
PROGMEM const uint8_t specia6[] = { 3, 0, 0, 1,  3, 1, 0, 2,  1, 0, 3,     0};
PROGMEM const uint8_t specia7[] = { 3, 2, 1, 5,  1, 3, 0,     1, 3, 6,     0};
PROGMEM const uint8_t specia8[] = { 3, 2, 1, 5,  1, 1, 0,     1, 1, 6,     0};
PROGMEM const uint8_t specia9[] = { 2, 3, 0, 4,  1, 1, 1,     1, 3, 1,     1, 2, 2,     1, 2, 4,     1, 1, 5,     1,3, 5,    0};
PROGMEM const uint8_t speci10[] = { 3, 2, 1, 5,  2, 3, 0, 4,  0};
PROGMEM const uint8_t speci11[] = { 1, 1, 6,     3, 1, 3, 4,  3, 2, 3, 5,  0};
PROGMEM const uint8_t speci12[] = { 2, 3, 0, 4,  0};
PROGMEM const uint8_t speci13[] = { 3, 0, 5, 6,  3, 1, 5, 6,  0};
PROGMEM const uint8_t speci14[] = { 1, 0, 5,     1, 1, 4,     1, 2, 3,     1, 3, 2,     1, 4, 1,     0};
PROGMEM const uint8_t speci15[] = { 2, 1, 0, 1,  2, 2, 0, 1,  2, 4, 0, 1,  2, 5, 0, 1,  0};
PROGMEM const uint8_t speci16[] = { 3, 0, 1, 2,  3, 1, 1, 2,  2, 4, 0, 1,  2, 5, 0, 1,  1, 0, 6,     0};
PROGMEM const uint8_t speci17[] = { 1, 1, 3,     1, 2, 2,     1, 3, 1,     1, 2, 4,     1, 3, 5,     0};
PROGMEM const uint8_t speci18[] = { 2, 2, 1, 3,  2, 4, 1, 3,  0};
PROGMEM const uint8_t speci19[] = { 1, 3, 3,     1, 2, 2,     1, 1, 1,     1, 2, 4,     1, 1, 5,     0};
PROGMEM const uint8_t speci20[] = { 2, 0, 1, 3,  1, 0, 1,     3, 4, 1, 2,  1, 3, 3,     1, 2, 4,     1, 2, 6,     0};
PROGMEM const uint8_t speci21[] = { 2, 0, 1, 3,  3, 0, 1, 5,  2, 6, 1, 3,  3, 4, 1, 4,  2, 2, 2, 3,  2, 4, 2, 3,  1, 2, 3,     0};


 const uint8_t *letter_table[] = {
 letterA,  //0
 letterB,
 letterC,
 letterD,
 letterE,
 letterF,
 letterG,
 letterH,
 letterI,
 letterJ,
 letterK,  //10
 letterL,
 letterM,
 letterN,
 letterO,
 letterP,
 letterQ,
 letterR,
 letterS,
 letterT,
 letterU,  //20
 letterV,
 letterW,
 letterX,
 letterY,
 letterZ,
 lettera,
 letterb,
 letterc,
 letterd,
 lettere,  //30
 letterf,
 letterg,
 letterh,
 letteri,
 letterj,
 letterk,
 letterl,
 letterm,
 lettern,
 lettero,  //40
 letterp,
 letterq,
 letterr,
 letters,
 lettert,
 letteru,
 letterv,
 letterw,
 letterx,
 lettery,  //50
 letterz,
 letter0,
 letter1,
 letter2,
 letter3,
 letter4,
 letter5,
 letter6,
 letter7,
 letter8,  //60
 letter9,
 specia0,
 specia1,
 specia2,
 specia3,
 specia4,
 specia5,
 specia6,
 specia7,
 specia8, //70
 specia9,
 speci10,
 speci11,
 speci12,
 speci13,  //75
 speci14,
 speci15,  //77
 speci16,
 speci17,
 speci18,  //80
 speci19,
 speci20, 
 speci21  //83
};

SoftwareSerial SoftS( SoftUartRx, SoftUartTx );
Adafruit_NeoPixel matrix = Adafruit_NeoPixel( PixelCount, NeoPixelPin, NEO_GRB + NEO_KHZ800 );

void Timer1start(uint8_t fp4s)
{
  OCR1A = 62500 / fp4s - 1;
  TCCR1B |= (1 << CS12) | (1 << CS10);
}

void Timer1stop()
{
 TCCR1B &= ~( (1 << CS12) | (1 << CS10) );
 TCNT1 = 0;
 TIFR1 = (1 << OCF1A);
}

void setup()
{
  /*Timer1 Setup*/
  TCNT1 = 0;
  OCR1A = 0; 
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  Timer1stop();
  
  pinMode( LEDPin, OUTPUT );  
  digitalWrite(LEDPin, LOW);
  
  Serial.begin(57600);
  SoftS.begin(9600);
  
  matrix.begin();
  matrix.show();
  /*Init global Variables*/  
  fp4s = 40;  
  
  mode = 2;
  lastmode = 2;
  
  message[0] = 8; message[1] = 68; message[2] = 38; message[3] = -1; message[4] = 26; message[5] = -1; message[6] = 33; message[7] = 26; message[8] = 45; message[9] = 75; message[40] = 10;

  colormode = 0;
  color = 0x00200000;
  
  for(uint8_t i = 0; i < 40; i++)
  {
    multi_color[i] = color;
  }
  /*Start Hat*/
  Timer1start(fp4s);  
}

void loop()
{
  static uint8_t init = 0b00000001;  //Bit[0] = new message; Bit[1] = init rainbow color
  if (mode == 0)
    init = menu();
  switch(mode)
  {
   case 1: testmode(init);  break;
   case 2: ticker(init); break;
  }
  init = 0;
  do
  {    
    if ( menueSerialavailable(-1) > 0 && mode != 0 )
    {
      menue_input_interface = 0;
      if (Serial.available())
        menue_input_interface = 1;
        
      lastmode = mode;
      mode = 0;
      
      for (uint8_t p = 0; p < PixelCount; p++)
      {
        matrix.setPixelColor( p, 0 ); 
      }
      digitalWrite(LEDPin, HIGH);
    }   
  } while(  !(TIFR1 & (1 << OCF1A) ) );
  TIFR1 = (1 << OCF1A);  
  
  matrix.show();
}

uint8_t menu()
{
  while( SoftS.available() > 0)
    SoftS.read();            /*Clear Input Stream */
  while( Serial.available() > 0)
    Serial.read();
    
  boolean menu = true;
  uint8_t change = 0;  
  mode = lastmode;
  
  do
  {
    menuePrintln(F("\n+++ LED-Hat menu +++"));
    menuePrintln(F("0:  Start with current settings"));
    menuePrintln(F("1:  Change fp4s"));
    menuePrintln(F("2:  Change mode"));
    menuePrintln(F("3:  Change message"));
    menuePrintln(F("4:  Change color mode"));
    menuePrintln(F("5:  Change color"));
    
    switch( SoftSintInput(0, 5) )
    {
      case 0:
        Timer1start(fp4s);
        menu = false;
        digitalWrite(LEDPin, LOW);
        break;
      
      case 1:
        menuePrintln(F("\nType your new fp4s (1 - 240):"));
        fp4s = SoftSintInput(1, 240);
        menuePrint(F("New fp4s is "));
        menuePrintDEC(fp4s);
        menuePrintln(F(".\n"));
        break;
        
      case 2:
        menuePrintln(F("\nSelect the new mode:"));
        menuePrintln(F("1:  Testmode"));
        menuePrintln(F("2:  Ticker"));
        mode = SoftSintInput(1, 2);
        if (mode != lastmode)
        {
         change |= (1 << 0);
         if (mode == 2 && (colormode == 2 || colormode == 4 || colormode == 6))
           change |= (1 << 1); 
        }
        menuePrint(F("New mode is "));
        menuePrintDEC(mode);        
        menuePrintln(F(".\n"));
        break;
      case 3:
        menuePrintln(F("\nChange the message([A-Z] [a-z] [0-9] [!\"#$%&\'()*+,-./]) :\n"));
        SoftSmessageInput(39);
        if( colormode != 0)
        {
          menuePrintln(F("\nPlease redefine your individual colors."));
          if( colormode == 2 || colormode == 4 || colormode == 6 )
            change |= (1 << 1);
        }
        
        for( uint8_t i = 0; i < message[40]; i++)
        {
          signed char nextChar = message[i];
          signed char nextChar_inlettertable;
          
          //to fit declaration in pointer
          if(nextChar >= 65 && nextChar <= 90)
          {
            nextChar_inlettertable = nextChar - 65; 
          }
          else if(nextChar >= 97 && nextChar <= 122)
          {
            nextChar_inlettertable = nextChar - 71;       
          }
          else if(nextChar >= 48 && nextChar <= 57)
          {
            nextChar_inlettertable = nextChar + 4;
          }
          else if(nextChar >= 33 && nextChar <= 47)
          {
            nextChar_inlettertable = nextChar + 29;
          }
          else if(nextChar >= 58 && nextChar <= 64)
          {
            nextChar_inlettertable = nextChar + 19;
          }
          else if(nextChar == 32)
          {
            nextChar_inlettertable = -1;
          }
          else
          {
            nextChar_inlettertable = -1; 
          }
          message[i] = nextChar_inlettertable;
        }
        change |= (1 << 0);        
        break;
      case 4:
        menuePrintln(F("\n+++ Select the color mode: +++"));
        menuePrintln(F("1: Single color applys to the whole message."));
        menuePrintln(F("2: Rainbow colors for each column."));
        menuePrintln(F("3: Individual colors for each column."));
        menuePrintln(F("4: Rainbow colors for each row."));
        menuePrintln(F("5: Individual colors for each row."));
        menuePrintln(F("6: Rainbow colors for each character."));
        menuePrintln(F("7: Individual colors for each character."));
        {
          uint8_t temp_color_mode = colormode;
          colormode = SoftSintInput(1, 7);
          if (colormode != temp_color_mode && (colormode == 2 || colormode == 4 || colormode == 6) )
            change |= (1 << 1);
        }
        menuePrint(F("New color mode is "));        
        menuePrintDEC(colormode);
        menuePrintln(F(".\n"));
        break;
      case 5:
        {
        uint8_t individual_color_length = 0;
        switch( colormode )
        {
        case 1:
          menuePrintln(F("\nChange the single color (RGB 0-255):\n"));
          color = matrix.Color(SoftSintInput(0,255), SoftSintInput(0,255), SoftSintInput(0,255) );
          menuePrintln(F("color set"));
          break;
          
        case 3:
          menuePrintln(F("1: Define color for each of the 34 colums?"));
          menuePrintln(F("2: No! The colormode will be changed back to single color."));
          if( SoftSintInput(1, 2) == 1)
          {
            individual_color_length = PixelWidth;
          }
          else
          {
            colormode = 1;
          }
          break;
        
        case 5:
          menuePrintln(F("1: Define color for each of the 7 rows?"));
          menuePrintln(F("2: No! The colormode will be changed back to single color."));
          if( SoftSintInput(1, 2) == 1)
          {
            individual_color_length = 7;
          }
          else
          {
            colormode = 1;
          }
          
          break;
          
        case 7:
          { 
            uint8_t msg_len = message_len_without_whitespaces();
            menuePrint(F("1: Define color for each of the "));
            menuePrintDEC(msg_len);
            menuePrintln(F(" characters?"));
            menuePrintln(F("2: No! The colormode will be changed back to single color."));
            if( SoftSintInput(1, 2) == 1)
            {
              individual_color_length = msg_len;
            }
            else
            {
              colormode = 1;
            }
          }
          break;
          
        default:
          menuePrintln(F("your color mode has no options to chose a color!"));
          break;
        }
        
        if( colormode != 0 )
        {
          menuePrintln(F("Define your multiple colors:"));
          for( uint8_t color_input_repeat = 0; color_input_repeat < individual_color_length; color_input_repeat++)
          {
            menuePrint(F("RGB Values Nr."));
            menuePrintDEC(color_input_repeat+1 );
            menuePrint(F(" :\n"));
            multi_color[color_input_repeat] = matrix.Color(SoftSintInput(0,255), SoftSintInput(0,255), SoftSintInput(0,255) );
          }
          menuePrintln(F("Finished, defining multiple colors."));
        }
        }        
        break;
    } 
  } while(menu);
  
  return change;
}

void menuePrintln(String msg)
{
 if (menue_input_interface == 0)
 {
  SoftS.println(msg);
 }
 else if (menue_input_interface == 1)
 {
  Serial.println(msg); 
 }
}

void menuePrint(String msg)
{
 if (menue_input_interface == 0)
 {
  SoftS.print(msg);
 }
 else if (menue_input_interface == 1)
 {
  Serial.print(msg); 
 }
}

void menuePrintDEC(uint8_t x)
{
if (menue_input_interface == 0)
  {
    SoftS.print(x, DEC);
  }
   else if (menue_input_interface == 1)
  {
    Serial.print(x, DEC);
  } 
}

uint8_t menueSerialavailable(int8_t interface)
{
  if(interface == -1)
   return SoftS.available() + Serial.available();
    
  else if (interface == 0)
    return SoftS.available();
  
  else if(interface == 1)
    return Serial.available();
}

char menueCharin(uint8_t interface)
{
  if (interface == 0)
    return SoftS.read();
  
  if (interface == 1)
    return Serial.read();
}

void SoftSmessageInput(uint8_t maxlength)
{
  menuePrint(F("Enter a text: "));
  for(uint8_t i = 0; i < 40; i++)
  {
   message[i] = 0; 
  }
  boolean waitinput = true;
  uint8_t length = 0;
 
  do
 {
  while(menueSerialavailable(menue_input_interface) > 0)
  {
    byte temp = menueCharin(menue_input_interface);
    if (temp == '\n' || length == maxlength)
    {
      message[40] = length;
      menuePrint(F("\nNew message is: "));
      uint8_t p = 0;
      char out;
      do
      {
        out = message[p++];
        menuePrint( (String)out );        
      }while(out != 0);
      waitinput = false;
      break;
    }    
    else if (temp == 127 && length > 0)
    {
      message[--length] = 0;
      menuePrint(F("<DEL>"));
    }
    else if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) || (temp >= 32 && temp <= 64 ) )
    {
      message[length++] = temp;
      if (menue_input_interface == 0)
      {
         SoftS.write(temp);
      }
      else if (menue_input_interface == 1)
      {
         Serial.write(temp); 
      } 
    }    
  }
  
 }while(waitinput); 
}

uint8_t message_len_without_whitespaces()
{
  uint8_t whitespacecount = 0;
  uint8_t i = 0;
  for( ; i < message[40]; i++)
    if( message[i] < 1)
      whitespacecount++;
  return i - whitespacecount; 
}

uint8_t SoftSintInput (uint8_t minv, uint8_t maxv)
{
  String input;
  uint8_t result;
  menuePrint(F("Enter a number: "));
  
  boolean waituntilvalidv = true;
  do
  {  
    boolean wait = true;  
    do
    {
      while( menueSerialavailable(menue_input_interface) > 0 && wait )
      {
        char tempIn = menueCharin(menue_input_interface);
        if( isDigit(tempIn) )
        {
          input += tempIn;
          menuePrint( (String)tempIn ); 
        }
        else if(tempIn == '\n' )
        {
           result = input.toInt();
           menuePrint(F(" -> "));
           menuePrintDEC(result);
           menuePrint(F("\n"));
           wait = false;           
        }
        else if(tempIn == 127 && input.length() > 0 )
        {
          input.remove( input.length() - 1, 1);
          menuePrint(F("<DEL>"));
        }
      }
    }while( wait );
      
    if( result >= minv && result <= maxv )
    {
      waituntilvalidv = false;
    }
    else
    {
      input = "";
      result = 0;
      menuePrint(F("NO WAY!\n Input must be between "));      
      menuePrintDEC( minv );
      menuePrint(F(" and "));
      menuePrintDEC( maxv );
      menuePrintln(F(". Try again!"));
    }
    
  }while( waituntilvalidv );  
  return result;
}

uint8_t xyToNum( uint8_t x, uint8_t y )
{
   uint8_t PNum;
   switch( y )  /* Use of switch case to avoid a mutiplication of x * PixelWidth */
   {
    case  0: PNum = 0 * PixelWidth;      break;
    case  1: PNum = 2 * PixelWidth - 1;  break;
    case  2: PNum = 2 * PixelWidth;      break;
    case  3: PNum = 4 * PixelWidth - 1;  break;
    case  4: PNum = 4 * PixelWidth;      break;
    case  5: PNum = 6 * PixelWidth - 1;  break;
    case  6: PNum = 6 * PixelWidth;      break;
   }
   if ( y & ( 1 << 0) )
   {
     PNum -= x;
   }
   else
   { 
     PNum += x; 
   }
   return PNum;
}

void drawhorizontalline(uint8_t y, uint8_t x1, uint8_t x2, const uint8_t *letter_num)
{
  uint8_t pixel = xyToNum(x1, y); 
  
  int8_t countd;
  if ( y & ( 1 << 0) )
  {
    countd = -1;
  }
  else
  {
    countd = 1; 
  }
   
  uint8_t x = x1;
  while(x <= x2) 
  {
    matrix.setPixelColor(pixel, *tickerColor(&x, &y, letter_num) );
    
    pixel += countd;
    x++;
  }
}

void drawverticalline(uint8_t x, uint8_t y1, uint8_t y2, const uint8_t *letter_num)
{
  uint8_t pixel = xyToNum(x, y1);
  
  matrix.setPixelColor(pixel, *tickerColor(&x, &y1, letter_num));
  
  uint8_t y = y1 + 1;
  while(y <= y2)
  {
    uint8_t tempadd;
    if ( y & ( 1 << 0) )
    {
      tempadd = ( ( (PixelWidth - 1) - x) << 1) + 1;
    }
    else
    {
      tempadd = (x << 1) + 1;
    }
    pixel += tempadd;
    
    matrix.setPixelColor(pixel, *tickerColor(&x, &y, letter_num) );
    
    y++;
  }
  
}

void testmode(uint8_t init)
{
  static uint8_t xPixel = 0;
  static uint8_t yPixel = 0;
  static uint8_t color = 255;
  if( init & (1 << 0) )
  {
    xPixel = 0;
    yPixel = 0;
    color = 0; 
  }
  else
  {
    if( color == 2)
    {
      color = 0;
      matrix.setPixelColor( xyToNum( xPixel, yPixel ), 0 );
      xPixel++;
      if( xPixel > 33)
      {
        xPixel = 0;
        yPixel++;
        if( yPixel > 6 ) 
        {
          yPixel = 0; 
        }
      }
    }
    else
    {
      color++;
    }
  }
  
  uint32_t newcolor;
  switch( color)
  {
   case 0: newcolor = matrix.Color(50, 0, 0); break;
   case 1: newcolor = matrix.Color(0, 50, 0); break;
   case 2: newcolor = matrix.Color(0, 0, 50); break;
  }  
  matrix.setPixelColor( xyToNum( xPixel, yPixel ), newcolor ); 
}

void init_tickerColor_rainbow()
{
  uint8_t color_steps = 0;
  switch( colormode ) 
  {
    case 2:
       color_steps = 32;
       break;
       
    case 4:
      color_steps = 7;
      break;
      
    case 6:
      color_steps = message_len_without_whitespaces();
      break;
    
    default:
      return;      
  }
  uint8_t temp = 255 / (color_steps -1);
  
  for(uint8_t current_step = 0; current_step < color_steps; current_step++ )
  {
    multi_color[current_step] = Wheel( current_step * temp );
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

uint32_t *tickerColor(const uint8_t *x, const uint8_t *y, const uint8_t *letter_num)
{
  static uint32_t nextcolor= color;  
  switch( colormode )
  {
   case 1:
     nextcolor = color;
     break;
     
   case 2:
   case 3:
     nextcolor = multi_color[*x];
     break;
     
   case 4:
   case 5:
     nextcolor = multi_color[*y];
     break;
     
   case 6:
   case 7:
     nextcolor = multi_color[*letter_num];
     break;
  }  
  return &nextcolor;
}

void ticker( uint8_t init)
{
  static int16_t textpos = -12;
  static uint8_t pixellength = 12;
 
 if ( init & (1 << 1) )
   init_tickerColor_rainbow();
 
 if( init & (1 << 0) )
 {
   textpos = -1 * (PixelWidth -1);
   pixellength = message[40] *6;
 }
 else
 {
   for (uint8_t p = 0; p < PixelCount; p++)
      {
        matrix.setPixelColor( p, 0 ); 
      }
   if (textpos == pixellength)
     textpos = -1 * (PixelWidth -1);
  textpos++;
 }
 
 boolean messageloopctrl = true;
 int8_t nextChar_inlettertable;
 uint8_t letterpos = 0;
 uint8_t i = 0;
 uint8_t character_without_whitespace = 255;
 
 while(messageloopctrl)
 {
   signed char next_pointer_toChar = message[i];
   
   if( message[40] == i)
   {
     messageloopctrl = false;
     break;
   }
   else if( (textpos - letterpos >= -1 * PixelWidth ) && (textpos - letterpos <= 4) )
   {
     if(next_pointer_toChar >= 0)
     {
       boolean letterloopctrl = true;     
       //const uint8_t *letteraddr = (uint8_t*)pgm_read_word_near( &(letter_table)[next_pointer_toChar] );
       const uint8_t *letteraddr = letter_table[next_pointer_toChar] ;
       int16_t arguments[3];
       uint8_t p = 0;
       character_without_whitespace++;
       
       while(letterloopctrl)
       {         
         uint8_t command = pgm_read_byte_near(&(letteraddr)[p++]);
         
         switch(command)
         {
          case 0:
            letterloopctrl = false;
            break;
            
          case 1:
            arguments[0] = letterpos + pgm_read_byte_near(&(letteraddr)[p++]) - textpos;
            
            if( arguments[0] >= 0 && arguments[0] <= PixelWidth)
            {
              uint8_t temp_arg0 = (uint8_t)arguments[0]; 
              uint8_t temp_arg1 = pgm_read_byte_near(&(letteraddr)[p++]);
              
              matrix.setPixelColor( xyToNum( temp_arg0, temp_arg1 ), *tickerColor(&temp_arg0, &temp_arg1, &character_without_whitespace) );
            }
            else
            {
              p++; 
            }
            
            break;
           
           case 2:
             
             arguments[0] = pgm_read_byte_near(&(letteraddr)[p++]);
             arguments[1] = letterpos + pgm_read_byte_near(&(letteraddr)[p++]) -textpos;
             arguments[2] = letterpos + pgm_read_byte_near(&(letteraddr)[p++]) -textpos;
             
               
             if( arguments[1] < 0)
             {
               if( arguments[2] >= 0)
               {
                 arguments[1] = 0;
                 drawhorizontalline(arguments[0], arguments[1], arguments[2], &character_without_whitespace);
               }
             }
             
             else if( arguments[2] > PixelWidth - 1)
             {
               if( arguments[1] <= PixelWidth -1)
               {
                 arguments[2] = PixelWidth - 1;
                 drawhorizontalline(arguments[0], arguments[1], arguments[2], &character_without_whitespace);
               } 
             }
             else
             {
               drawhorizontalline(arguments[0], arguments[1], arguments[2], &character_without_whitespace);
             }                   
             
             break;
            
            case 3:
              arguments[0] = letterpos + pgm_read_byte_near(&(letteraddr)[p++]) -textpos;
              
              if(arguments[0] >= 0 && arguments[0] <= PixelWidth -1)
              { 
                arguments[1] = pgm_read_byte_near(&(letteraddr)[p++]);
                arguments[2] = pgm_read_byte_near(&(letteraddr)[p++]);
                
                
                drawverticalline(arguments[0], arguments[1], arguments[2], &character_without_whitespace);
              }
              else
              {
                p += 2;
              }
              
              break;
            } 
         }
     }     
   }
   else
   {
     if(next_pointer_toChar > 0)
       character_without_whitespace++;
   }
   letterpos += 6;
   i++;
   }   
 }
