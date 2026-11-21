#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

/*Pin definitions*/
#define SoftUartRx   10
#define SoftUartTx   11
#define BLstatus     8
#define LEDPin       13
#define NeoPixelPin  5

/*Matrix x,y width, height*/
#define PixelWidth  37
#define PixelHeight 7

/*Pixel count, 259: more than a uint8_t holds, so pixel indices must be uint16_t*/
#define PixelCount  (PixelWidth * PixelHeight)

/*Frames per 4 seconds*/
uint8_t fp4s = 40;

/*OperationModes*/
/*0=menu ; 1=testmode ; 2=ticker ; 3=dancing banana ; 4=pac-man ; 5=space invaders ;
  6=fire ; 7=plasma ; 8=game of life ; 9=snow*/
uint8_t mode = 1;
uint8_t lastmode = 1;
uint8_t menue_input_interface; /* 0=bluetooth 1= serial */

/*Text Ticker Message*/
signed char message[41]; //Byte[40] is reserved for length

/*color modes for Text Ticker*/
uint8_t colormode = 0;
uint32_t color;            //mode = 0;
uint32_t multi_color[40];  //mode = 1, 2

/*A grey column marks the seam while a message too long to loop around the hat
  scrolls in and out there. The seam is at the exact back of the hat. The matrix sits
  one column off center, so that is the second to last column rather than the last one.*/
#define TickerSeamColumn (PixelWidth - 2)


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

/*Dancing banana sprite, 7x7 per frame. ' ' = off, 'S' = stem, 'L' = arms and legs,
  body shaded as if lit from the left: 'H' = highlight, 'Y' = base, 'D' = shadow*/
#define BananaWidth   7
#define BananaFrames  4
#define BananaCount   3   //bananas on the hat, spread evenly around it
PROGMEM const char banana_frames[BananaFrames][PixelHeight][BananaWidth + 1] = {
  { "  S    ",    //0: lean left, arms up, jump
    "L HD  L",
    " LHYDL ",
    "  HYD  ",
    "   YD  ",
    "  L L  ",
    " L   L " },
  { "   S   ",    //1: upright, arms out
    "   HD  ",
    "LLHYDLL",
    "  HYD  ",
    "  YD   ",
    "  L L  ",
    "  L L  " },
  { "    S  ",    //2: lean right, arms up, jump
    "L  HD L",
    " LHYDL ",
    "  HYD  ",
    "  YD   ",
    "  L L  ",
    " L   L " },
  { "   S   ",    //3: upright, arms down, knees bent
    "   HD  ",
    " LHYDL ",
    "L HYD L",
    "  YD   ",
    " L  L  ",
    " L  L  " }
};
/*Pac-Man sprites, 7x7, facing right (mirrored for left). 'P' = Pac-Man,
  ghost: 'G' = body, 'W' = eye white, 'E' = pupil; frightened ghost: 'B' = body, 'F' = face*/
#define PacWidth  7
PROGMEM const char pac_frames[3][PixelHeight][PacWidth + 1] = {
  { "  PPP  ",    //0: mouth closed
    " PPPPP ",
    "PPPPPPP",
    "PPPPPPP",
    "PPPPPPP",
    " PPPPP ",
    "  PPP  " },
  { "  PPP  ",    //1: half open
    " PPPPP ",
    "PPPPPPP",
    "PPPP   ",
    "PPPPPPP",
    " PPPPP ",
    "  PPP  " },
  { "  PPP  ",    //2: open
    " PPPPP ",
    "PPPP   ",
    "PPP    ",
    "PPPP   ",
    " PPPPP ",
    "  PPP  " }
};
PROGMEM const char ghost_frames[2][PixelHeight][PacWidth + 1] = {
  { "  GGG  ",
    " GGGGG ",
    "GWWGWWG",
    "GWEGWEG",
    "GGGGGGG",
    "GGGGGGG",
    "G G G G" },
  { "  GGG  ",
    " GGGGG ",
    "GWWGWWG",
    "GWEGWEG",
    "GGGGGGG",
    "GGGGGGG",
    " G G G " }
};
PROGMEM const char ghost_fright[2][PixelHeight][PacWidth + 1] = {
  { "  BBB  ",
    " BBBBB ",
    "BBBBBBB",
    "BFBBBFB",
    "BBBBBBB",
    "BFBFBFB",
    "B B B B" },
  { "  BBB  ",
    " BBBBB ",
    "BBBBBBB",
    "BFBBBFB",
    "BBBBBBB",
    "BFBFBFB",
    " B B B " }
};
#define PacScoreWidth  11
PROGMEM const char pac_score[5][PacScoreWidth + 1] = {
  "XXX XXX XXX",
  "  X X X X X",
  "XXX X X X X",
  "X   X X X X",
  "XXX XXX XXX"
};
/*Positions are in quarter columns around the hat, so speeds can differ by less than a column per frame*/
#define PacRing        (PixelWidth * 4)
#define PacPelletRow   3
#define PacGhostStart  14   //columns the ghost starts behind Pac-Man
#define PacPowerAhead  22   //columns from Pac-Man to the power pellet
#define PacCatchGap    5    //sprite centers this close = ghost eaten (they overlap by 2 columns)
#define PacFlashAfter  9    //frames of fright before the ghost starts flashing
/*Phases*/
#define PacChase   0
#define PacFright  1
#define PacScore   2
#define PacEyes    3
/*Palettes*/
#define PalPac     0
#define PalGhost   1
#define PalEyes    2
#define PalFright  3
#define PalFlash   4
#define PalScore   5
#define PalSquid   6
#define PalCrab    7
#define PalBoom    8
#define PalCannon  9

/*Space invaders, 5x3, two animation frames each. 'X' = body, 'E' = eyes*/
#define InvWidth    5
#define InvHeight   3
#define InvCount    4   //invaders in the formation
#define InvSpacing  7   //columns from one invader to the next
#define InvRange    (PixelWidth - (InvCount - 1) * InvSpacing - InvWidth)   //columns the formation can march
#define InvLowest   2   //lowest formation row before the invaders land on the cannon
PROGMEM const char invader_frames[2][2][InvHeight][InvWidth + 1] = {
  { { " XXX ",      //squid
      "XEXEX",
      " X X " },
    { " XXX ",
      "XEXEX",
      "X   X" } },
  { { "X   X",      //crab
      "XEXEX",
      " X X " },
    { " X X ",
      "XEXEX",
      "X   X" } }
};
PROGMEM const char invader_boom[InvHeight][InvWidth + 1] = {
  "X X X",
  " X X ",
  "X X X"
};
PROGMEM const char cannon_sprite[2][3 + 1] = {
  " X ",
  "XXX"
};
/*sin(i * 2pi / 256) scaled to 0..255*/
PROGMEM const uint8_t sine_table[256] = {
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
  176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
  245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
  255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
  245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
  218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
  176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
  128, 124, 121, 118, 115, 112, 109, 106, 103, 100,  97,  93,  90,  88,  85,  82,
   79,  76,  73,  70,  67,  65,  62,  59,  57,  54,  52,  49,  47,  44,  42,  40,
   37,  35,  33,  31,  29,  27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,
   10,   9,   7,   6,   5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
   10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
   37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
   79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124
};

#define FireTicksPerStep  1
#define LifeTicksPerGen   4     //frames per generation
#define LifeMaxGens       200   //reseed after this many generations even if still changing
#define LifePauseGens     6     //generations to show the final state before reseeding
#define LifeDensity       35    //percent of cells alive when seeding
#define SnowFlakes        24

/*Scratch memory shared by the procedural effects, only the current mode uses it.
  RAM is tight: the NeoPixel buffer alone takes 3 bytes per pixel.*/
union
{
  uint8_t heat[(PixelCount + 1) / 2];       //fire: 4 bits per pixel
  uint8_t life[3][(PixelCount + 7) / 8];    //game of life: 1 bit per cell, 3 generations
  struct
  {
    uint8_t x[SnowFlakes];                  //quarter columns
    uint8_t y[SnowFlakes];                  //sixteenth rows
    uint8_t speed[SnowFlakes];
    uint8_t pile[PixelWidth];               //snow lying on the bottom row
  } snow;
} effect;

PROGMEM const char cannon_boom[2][2][3 + 1] = {
  { "X X",
    " X " },
  { " X ",
    "X X" }
};

#define BananaTicksPerStep  3   //frames per dance step
#define BananaStepsPerShift 4   //dance steps per column the troupe moves around the hat

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
   case 3: banana(init); break;
   case 4: pacman(init); break;
   case 5: invaders(init); break;
   case 6: fire(init); break;
   case 7: plasma(init); break;
   case 8: life(init); break;
   case 9: snow(init); break;
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
      
      clearMatrix();
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
        menuePrintln(F("3:  Dancing banana"));
        menuePrintln(F("4:  Pac-Man"));
        menuePrintln(F("5:  Space Invaders"));
        menuePrintln(F("6:  Fire"));
        menuePrintln(F("7:  Plasma"));
        menuePrintln(F("8:  Game of Life"));
        menuePrintln(F("9:  Snow"));
        mode = SoftSintInput(1, 9);
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
          menuePrint(F("1: Define color for each of the "));
          menuePrintDEC(PixelWidth);
          menuePrintln(F(" colums?"));
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
    if( message[i] < 0)
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

/*Serpentine wiring, starting top right: even rows run right to left, odd rows left to right*/
uint16_t xyToNum( uint8_t x, uint8_t y )
{
   uint16_t PNum;
   switch( y )  /* Use of switch case to avoid a mutiplication of x * PixelWidth */
   {
    case  0: PNum = 1 * PixelWidth - 1;  break;
    case  1: PNum = 1 * PixelWidth;      break;
    case  2: PNum = 3 * PixelWidth - 1;  break;
    case  3: PNum = 3 * PixelWidth;      break;
    case  4: PNum = 5 * PixelWidth - 1;  break;
    case  5: PNum = 5 * PixelWidth;      break;
    case  6: PNum = 7 * PixelWidth - 1;  break;
   }
   if ( y & ( 1 << 0) )
   {
     PNum += x;
   }
   else
   {
     PNum -= x;
   }
   return PNum;
}

void drawhorizontalline(uint8_t y, uint8_t x1, uint8_t x2, const uint8_t *letter_num)
{
  uint16_t pixel = xyToNum(x1, y);
  
  int8_t countd;
  if ( y & ( 1 << 0) )
  {
    countd = 1;
  }
  else
  {
    countd = -1;
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
  uint16_t pixel = xyToNum(x, y1);
  
  matrix.setPixelColor(pixel, *tickerColor(&x, &y1, letter_num));
  
  uint8_t y = y1 + 1;
  while(y <= y2)
  {
    uint8_t tempadd;
    if ( y & ( 1 << 0) )
    {
      tempadd = (x << 1) + 1;
    }
    else
    {
      tempadd = ( ( (PixelWidth - 1) - x) << 1) + 1;
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
      if( xPixel >= PixelWidth )
      {
        xPixel = 0;
        yPixel++;
        if( yPixel >= PixelHeight )
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

void banana(uint8_t init)
{
  static uint8_t tick = 0;
  static uint8_t step = 0;
  static uint8_t shift = 0;
  if( init & (1 << 0) )
  {
    tick = 0;
    step = 0;
    shift = 0;
  }
  else if( ++tick >= BananaTicksPerStep )
  {
    tick = 0;
    step++;
    if( step % BananaStepsPerShift == 0 )
      shift = (shift + 1) % PixelWidth;
  }

  clearMatrix();

  uint8_t frame = step % BananaFrames;
  for( uint8_t b = 0; b < BananaCount; b++ )
  {
    for( uint8_t y = 0; y < PixelHeight; y++ )
    {
      for( uint8_t x = 0; x < BananaWidth; x++ )
      {
        /*Every other banana is mirrored, so neighbours lean in opposite directions.
          Highlight and shadow swap sides too, to keep the light coming from the left.*/
        boolean mirrored = b & 1;
        uint8_t sx = mirrored ? BananaWidth - 1 - x : x;
        char c = pgm_read_byte_near( &banana_frames[frame][y][sx] );
        if( mirrored && c == 'H' )
          c = 'D';
        else if( mirrored && c == 'D' )
          c = 'H';
        uint32_t newcolor;
        switch( c )
        {
          case 'H': newcolor = matrix.Color(60, 52, 10); break;
          case 'Y': newcolor = matrix.Color(40, 30, 0);  break;
          case 'D': newcolor = matrix.Color(18, 11, 0);  break;
          case 'L': newcolor = matrix.Color(30, 8, 0);   break;
          case 'S': newcolor = matrix.Color(6, 20, 0);   break;
          default: continue;
        }
        /*The matrix wraps around the hat, so the troupe can dance all the way round*/
        uint8_t px = ( b * PixelWidth / BananaCount + x + shift ) % PixelWidth;
        matrix.setPixelColor( xyToNum( px, y ), newcolor );
      }
    }
  }
}

uint32_t spriteColor(char c, uint8_t palette)
{
  switch( palette )
  {
    case PalPac:
      if( c == 'P' ) return matrix.Color(50, 40, 0);
      break;
    case PalGhost:
      if( c == 'G' ) return matrix.Color(50, 0, 0);
      /*fall through for the eyes*/
    case PalEyes:
      if( c == 'W' ) return matrix.Color(30, 30, 30);
      if( c == 'E' ) return matrix.Color(0, 0, 45);
      break;
    case PalFright:
      if( c == 'B' ) return matrix.Color(0, 0, 45);
      if( c == 'F' ) return matrix.Color(40, 20, 20);
      break;
    case PalFlash:
      if( c == 'B' ) return matrix.Color(35, 35, 35);
      if( c == 'F' ) return matrix.Color(45, 0, 0);
      break;
    case PalScore:
      if( c == 'X' ) return matrix.Color(0, 35, 35);
      break;
    case PalSquid:
      if( c == 'X' ) return matrix.Color(40, 0, 40);
      if( c == 'E' ) return matrix.Color(45, 0, 0);
      break;
    case PalCrab:
      if( c == 'X' ) return matrix.Color(0, 35, 35);
      if( c == 'E' ) return matrix.Color(45, 0, 0);
      break;
    case PalBoom:
      if( c == 'X' ) return matrix.Color(45, 20, 0);
      break;
    case PalCannon:
      if( c == 'X' ) return matrix.Color(0, 45, 0);
      break;
  }
  return 0;
}

/*Draws a PROGMEM sprite (h rows of w chars plus terminator) centered on column cx,
  starting at row y0, wrapping around the hat. Characters without a color are transparent.*/
void drawWrappedSprite(const char *sprite, uint8_t w, uint8_t h, uint8_t y0, uint8_t cx, boolean mirrored, uint8_t palette)
{
  for( uint8_t y = 0; y < h; y++ )
  {
    for( uint8_t x = 0; x < w; x++ )
    {
      uint8_t sx = mirrored ? w - 1 - x : x;
      uint32_t newcolor = spriteColor( pgm_read_byte_near( &sprite[y * (w + 1) + sx] ), palette );
      if( newcolor == 0 )
        continue;
      uint8_t px = ( cx + PixelWidth - w / 2 + x ) % PixelWidth;
      matrix.setPixelColor( xyToNum( px, y0 + y ), newcolor );
    }
  }
}

int16_t pacWrap(int16_t pos)
{
  return ( (pos % PacRing) + PacRing ) % PacRing;
}

void pacman(uint8_t init)
{
  static uint8_t phase = PacChase;
  static int16_t pacPos = 0;
  static int16_t ghostPos = 0;
  static int8_t pacDir = 1;
  static uint8_t tick = 0;
  static uint8_t timer = 0;
  static uint8_t powerCol = 0;    //255 = eaten
  static uint8_t scoreCol = 0;
  static uint8_t pellets[(PixelWidth + 7) / 8];
  boolean newround = false;

  if( init & (1 << 0) )
  {
    pacPos = 3 * 4;
    tick = 0;
    newround = true;
  }
  else
  {
    tick++;
    switch( phase )
    {
      case PacChase:    /*the ghost is slightly faster and closes in*/
        pacPos = pacWrap( pacPos + pacDir * 2 );
        ghostPos = pacWrap( ghostPos + pacDir * ( (tick & 1) ? 3 : 2 ) );
        if( pacPos / 4 == powerCol )
        {
          phase = PacFright;
          timer = 0;
          powerCol = 255;
          pacDir = -pacDir;
        }
        break;

      case PacFright:   /*both turn round, the ghost flees at half speed*/
        pacPos = pacWrap( pacPos + pacDir * 2 );
        ghostPos = pacWrap( ghostPos + pacDir );
        timer++;
        if( pacWrap( pacDir * (ghostPos - pacPos) ) <= PacCatchGap * 4 )
        {
          phase = PacScore;
          timer = 0;
          scoreCol = ghostPos / 4;
        }
        break;

      case PacScore:    /*freeze and show the points, like the arcade*/
        if( ++timer >= 12 )
        {
          phase = PacEyes;
          timer = 0;
        }
        break;

      case PacEyes:     /*the eyes race off round the hat*/
        pacPos = pacWrap( pacPos + pacDir * 2 );
        ghostPos = pacWrap( ghostPos - pacDir * 6 );
        if( ++timer >= 24 )
          newround = true;
        break;
    }
  }

  if( newround )
  {
    phase = PacChase;
    pacDir = 1;
    ghostPos = pacWrap( pacPos - PacGhostStart * 4 );
    powerCol = ( pacPos / 4 + PacPowerAhead ) % PixelWidth;
    for( uint8_t i = 0; i < sizeof(pellets); i++ )
      pellets[i] = 0xFF;
  }

  uint8_t pacCol = pacPos / 4;
  pellets[pacCol >> 3] &= ~( 1 << (pacCol & 7) );

  clearMatrix();

  /*Pellets every third column, leaving room around the power pellet*/
  for( uint8_t x = 0; x < PixelWidth; x += 3 )
  {
    uint8_t d = ( x + PixelWidth - powerCol ) % PixelWidth;
    if( powerCol != 255 && (d <= 2 || d >= PixelWidth - 2) )
      continue;
    if( pellets[x >> 3] & ( 1 << (x & 7) ) )
      matrix.setPixelColor( xyToNum( x, PacPelletRow ), matrix.Color(20, 12, 6) );
  }
  if( powerCol != 255 && (tick & 4) )
  {
    uint32_t power = matrix.Color(45, 28, 14);
    matrix.setPixelColor( xyToNum( powerCol, PacPelletRow - 1 ), power );
    matrix.setPixelColor( xyToNum( powerCol, PacPelletRow + 1 ), power );
    matrix.setPixelColor( xyToNum( (powerCol + PixelWidth - 1) % PixelWidth, PacPelletRow ), power );
    matrix.setPixelColor( xyToNum( powerCol, PacPelletRow ), power );
    matrix.setPixelColor( xyToNum( (powerCol + 1) % PixelWidth, PacPelletRow ), power );
  }

  uint8_t skirt = (tick >> 2) & 1;
  switch( phase )
  {
    case PacChase:
      drawWrappedSprite( &ghost_frames[skirt][0][0], PacWidth, PixelHeight, 0, ghostPos / 4, pacDir < 0, PalGhost );
      break;
    case PacFright:
      drawWrappedSprite( &ghost_fright[skirt][0][0], PacWidth, PixelHeight, 0, ghostPos / 4, false,
                         (timer >= PacFlashAfter && (tick & 2)) ? PalFlash : PalFright );
      break;
    case PacScore:
      drawWrappedSprite( &pac_score[0][0], PacScoreWidth, 5, 1, scoreCol, false, PalScore );
      return;   //Pac-Man is hidden while the score shows
    case PacEyes:
      drawWrappedSprite( &ghost_frames[0][0][0], PacWidth, PixelHeight, 0, ghostPos / 4, pacDir > 0, PalEyes );
      break;
  }

  static const uint8_t mouth[4] = { 0, 1, 2, 1 };
  drawWrappedSprite( &pac_frames[ mouth[(tick >> 1) & 3] ][0][0], PacWidth, PixelHeight, 0, pacCol, pacDir < 0, PalPac );
}

uint8_t invaderAliveCount(uint8_t alive)
{
  uint8_t n = 0;
  for( uint8_t i = 0; i < InvCount; i++ )
    if( alive & (1 << i) )
      n++;
  return n;
}

/*Picks a random living invader, or 255 if there are none*/
uint8_t randomInvader(uint8_t alive)
{
  uint8_t n = invaderAliveCount(alive);
  if( n == 0 )
    return 255;
  uint8_t k = random(n);
  for( uint8_t i = 0; i < InvCount; i++ )
    if( (alive & (1 << i)) && k-- == 0 )
      return i;
  return 255;
}

void invaders(uint8_t init)
{
  static uint8_t alive = 0;       //bit i = invader i
  static uint8_t fx = 0, fy = 0;  //formation position: left column, top row
  static int8_t fdir = 1;
  static uint8_t anim = 0;
  static uint8_t stepTimer = 0;
  static uint8_t cannonX = 0;
  static uint8_t cannonDead = 0;  //frames left of the cannon's explosion
  static uint8_t target = 0;
  static int8_t laserX = -1, laserY = -1;   //-1 = no laser
  static int8_t bombX = -1, bombY = -1;     //-1 = no bomb
  static uint8_t boomX = 0, boomY = 0, boomTimer = 0;
  static uint8_t waveTimer = 0;   //pause before the next wave
  static uint8_t tick = 0;

  if( init & (1 << 0) )
  {
    cannonX = PixelWidth / 2;
    cannonDead = 0;
    waveTimer = 1;
    alive = 0;
  }
  tick++;

  /*New wave*/
  if( waveTimer > 0 && --waveTimer == 0 )
  {
    alive = (1 << InvCount) - 1;
    fx = InvRange / 2;
    fy = 0;
    fdir = 1;
    stepTimer = 0;
    laserY = -1;
    bombY = -1;
    target = randomInvader(alive);
  }

  /*The formation marches, faster the fewer are left, and steps down at the ends*/
  if( alive && ++stepTimer >= 2 + invaderAliveCount(alive) )
  {
    stepTimer = 0;
    anim ^= 1;
    if( (fdir > 0 && fx >= InvRange) || (fdir < 0 && fx == 0) )
    {
      fdir = -fdir;
      if( ++fy > InvLowest )
      {
        /*Landed: the invaders win this wave*/
        for( uint8_t i = 0; i < InvCount; i++ )
          if( alive & (1 << i) )
          {
            boomX = fx + i * InvSpacing + InvWidth / 2;
            boomY = InvLowest;
          }
        boomTimer = 8;
        alive = 0;
        cannonDead = 20;
        waveTimer = 30;
      }
    }
    else
    {
      fx += fdir;
    }
  }

  /*Laser: one row per frame, hits an invader where it has a pixel*/
  if( laserY >= 0 )
  {
    laserY--;
    if( laserY >= fy && laserY < fy + InvHeight )
    {
      for( uint8_t i = 0; i < InvCount; i++ )
      {
        uint8_t sx = ( laserX + PixelWidth - fx - i * InvSpacing ) % PixelWidth;
        if( !(alive & (1 << i)) || sx >= InvWidth )
          continue;
        if( pgm_read_byte_near( &invader_frames[i & 1][anim][laserY - fy][sx] ) != ' ' )
        {
          alive &= ~(1 << i);
          boomX = fx + i * InvSpacing + InvWidth / 2;
          boomY = fy;
          boomTimer = 6;
          laserY = -1;
          target = randomInvader(alive);
          if( !alive )
            waveTimer = 25;
          break;
        }
      }
    }
  }

  /*Bombs: one at a time, half the laser's speed*/
  if( bombY >= 0 )
  {
    if( tick & 1 )
      bombY++;
    if( bombY >= PixelHeight - 2 && !cannonDead && bombX >= cannonX - 1 && bombX <= cannonX + 1 )
    {
      cannonDead = 16;
      bombY = -1;
    }
    else if( bombY >= PixelHeight )
    {
      bombY = -1;
    }
  }
  else if( alive && random(12) == 0 )
  {
    uint8_t i = randomInvader(alive);
    bombX = fx + i * InvSpacing + InvWidth / 2;
    bombY = fy + InvHeight;
  }

  /*Cannon: hunts its target and fires when lined up*/
  if( cannonDead > 0 )
  {
    cannonDead--;
  }
  else if( alive && target != 255 )
  {
    uint8_t goal = fx + target * InvSpacing + InvWidth / 2;
    if( (tick & 1) && cannonX < goal )
      cannonX++;
    else if( (tick & 1) && cannonX > goal )
      cannonX--;
    else if( cannonX == goal && laserY < 0 )
    {
      laserX = cannonX;
      laserY = PixelHeight - 2;
    }
  }

  /*Draw*/
  clearMatrix();
  for( uint8_t i = 0; i < InvCount; i++ )
    if( alive & (1 << i) )
      drawWrappedSprite( &invader_frames[i & 1][anim][0][0], InvWidth, InvHeight, fy,
                         fx + i * InvSpacing + InvWidth / 2, false, (i & 1) ? PalCrab : PalSquid );
  if( boomTimer > 0 )
  {
    boomTimer--;
    drawWrappedSprite( &invader_boom[0][0], InvWidth, InvHeight, boomY, boomX, false, PalBoom );
  }
  if( laserY >= 0 && laserY < PixelHeight - 2 )
    matrix.setPixelColor( xyToNum( laserX, laserY ), matrix.Color(40, 40, 40) );
  if( bombY >= 0 && bombY < PixelHeight )
    matrix.setPixelColor( xyToNum( bombX, bombY ), matrix.Color(45, 10, 0) );
  if( cannonDead > 0 )
    drawWrappedSprite( &cannon_boom[(tick >> 1) & 1][0][0], 3, 2, PixelHeight - 2, cannonX, false, PalBoom );
  else
    drawWrappedSprite( &cannon_sprite[0][0], 3, 2, PixelHeight - 2, cannonX, false, PalCannon );
}

void clearMatrix()
{
  for (uint16_t p = 0; p < PixelCount; p++)
  {
    matrix.setPixelColor( p, 0 );
  }
}

uint8_t sin8(uint8_t angle)
{
  return pgm_read_byte_near( &sine_table[angle] );
}

/*Fire: heat rises from the bottom row, spreads sideways and cools*/
uint8_t heatGet(uint8_t x, uint8_t y)
{
  uint16_t i = y * PixelWidth + x;
  return ( effect.heat[i >> 1] >> ((i & 1) << 2) ) & 0x0F;
}

void heatSet(uint8_t x, uint8_t y, uint8_t h)
{
  uint16_t i = y * PixelWidth + x;
  uint8_t shift = (i & 1) << 2;
  effect.heat[i >> 1] = ( effect.heat[i >> 1] & ~(0x0F << shift) ) | (h << shift);
}

uint32_t heatColor(uint8_t h)
{
  if( h <= 5 )
    return matrix.Color(h * 10, 0, 0);                              //black to red
  if( h <= 10 )
    return matrix.Color(50, (h - 5) * 6, 0);                        //red to orange
  return matrix.Color(55, 30 + (h - 10) * 4, (h - 10) * 3);         //orange to yellow-white
}

void fire(uint8_t init)
{
  if( init & (1 << 0) )
    memset( effect.heat, 0, sizeof(effect.heat) );

  /*Top down, so each row still reads the old values of the row below it*/
  for( uint8_t y = 0; y < PixelHeight - 1; y++ )
  {
    for( uint8_t x = 0; x < PixelWidth; x++ )
    {
      uint8_t sum = heatGet( (x + PixelWidth - 1) % PixelWidth, y + 1 ) + 2 * heatGet( x, y + 1 )
                  + heatGet( (x + 1) % PixelWidth, y + 1 );
      uint8_t h = sum / 4;
      uint8_t cool = random(5);
      heatSet( x, y, h > cool ? h - cool : 0 );
    }
  }
  /*Fuel: the bottom row flickers randomly*/
  for( uint8_t x = 0; x < PixelWidth; x++ )
    heatSet( x, PixelHeight - 1, random(5, 16) );

  for( uint8_t y = 0; y < PixelHeight; y++ )
    for( uint8_t x = 0; x < PixelWidth; x++ )
      matrix.setPixelColor( xyToNum( x, y ), heatColor( heatGet( x, y ) ) );
}

/*Plasma: overlapping sine waves mapped to the color wheel. The x terms use whole
  periods around the hat, so there is no seam where the matrix wraps.*/
void plasma(uint8_t init)
{
  static uint8_t t1 = 0, t2 = 0, t3 = 0;
  t1 += 3;
  t2 += 2;
  t3 -= 1;

  for( uint8_t y = 0; y < PixelHeight; y++ )
  {
    for( uint8_t x = 0; x < PixelWidth; x++ )
    {
      uint8_t a = (uint16_t)x * 256 / PixelWidth;
      uint16_t v = sin8( (uint8_t)(a * 2 + t1) ) + sin8( (uint8_t)(y * 37 + t2) )
                 + sin8( (uint8_t)(a + y * 23 + t3) ) + sin8( (uint8_t)(a * 3 - y * 16 - t2) );
      uint32_t c = Wheel( (uint8_t)(v >> 2) + t3 );
      matrix.setPixelColor( xyToNum( x, y ), (c >> 2) & 0x3F3F3F );
    }
  }
}

/*Conway's Game of Life on a cylinder: columns wrap around the hat, top and bottom are edges*/
boolean lifeGet(const uint8_t *grid, int8_t x, int8_t y)
{
  if( y < 0 || y >= PixelHeight )
    return false;
  uint16_t i = y * PixelWidth + (x + PixelWidth) % PixelWidth;
  return grid[i >> 3] & (1 << (i & 7));
}

void lifeSet(uint8_t *grid, uint8_t x, uint8_t y, boolean alive)
{
  uint16_t i = y * PixelWidth + x;
  if( alive )
    grid[i >> 3] |= (1 << (i & 7));
  else
    grid[i >> 3] &= ~(1 << (i & 7));
}

void life(uint8_t init)
{
  static uint8_t cur = 0;       //which of the 3 grids holds the current generation
  static uint8_t gen = 0;
  static uint8_t timer = 0;
  static uint8_t pause = 0;     //generations left before reseeding
  boolean reseed = init & (1 << 0);

  if( !reseed && ++timer >= LifeTicksPerGen )
  {
    timer = 0;
    if( pause > 0 )
    {
      reseed = --pause == 0;
    }
    else
    {
      uint8_t *now = effect.life[cur];
      uint8_t *prev = effect.life[(cur + 2) % 3];
      uint8_t *next = effect.life[(cur + 1) % 3];   //overwrites the oldest generation
      uint16_t population = 0;
      for( uint8_t y = 0; y < PixelHeight; y++ )
      {
        for( uint8_t x = 0; x < PixelWidth; x++ )
        {
          uint8_t n = 0;
          for( int8_t dy = -1; dy <= 1; dy++ )
            for( int8_t dx = -1; dx <= 1; dx++ )
              if( (dx || dy) && lifeGet( now, x + dx, y + dy ) )
                n++;
          boolean alive = n == 3 || (n == 2 && lifeGet( now, x, y ));
          lifeSet( next, x, y, alive );
          if( alive )
            population++;
        }
      }
      /*Stale: extinct, still life, blinking between two states, or just running too long*/
      if( population == 0 || !memcmp( next, now, sizeof(effect.life[0]) )
          || !memcmp( next, prev, sizeof(effect.life[0]) ) || ++gen >= LifeMaxGens )
        pause = LifePauseGens;
      cur = (cur + 1) % 3;
    }
  }

  if( reseed )
  {
    memset( effect.life, 0, sizeof(effect.life) );
    cur = 0;
    gen = 0;
    pause = 0;
    for( uint8_t y = 0; y < PixelHeight; y++ )
      for( uint8_t x = 0; x < PixelWidth; x++ )
        lifeSet( effect.life[cur], x, y, random(100) < LifeDensity );
  }

  /*Newborn cells are bright, survivors teal, cells that just died glow dim red*/
  const uint8_t *now = effect.life[cur];
  const uint8_t *prev = effect.life[(cur + 2) % 3];
  for( uint8_t y = 0; y < PixelHeight; y++ )
  {
    for( uint8_t x = 0; x < PixelWidth; x++ )
    {
      boolean a = lifeGet( now, x, y ), b = lifeGet( prev, x, y );
      uint32_t c = 0;
      if( a && !b )
        c = matrix.Color(20, 45, 30);
      else if( a )
        c = matrix.Color(0, 25, 20);
      else if( b )
        c = matrix.Color(10, 0, 3);
      matrix.setPixelColor( xyToNum( x, y ), c );
    }
  }
}

/*Snow: flakes at three depths drift in a slowly turning wind and pile up on the bottom row*/
void snowRespawn(uint8_t i, uint8_t y)
{
  effect.snow.x[i] = random(PixelWidth * 4);
  effect.snow.y[i] = y;
  effect.snow.speed[i] = random(1, 4);
}

void snow(uint8_t init)
{
  static uint16_t tick = 0;
  if( init & (1 << 0) )
  {
    for( uint8_t i = 0; i < SnowFlakes; i++ )
      snowRespawn( i, random((PixelHeight - 1) * 16) );
    memset( effect.snow.pile, 0, sizeof(effect.snow.pile) );
  }
  tick++;
  int16_t wind = (int16_t)sin8( tick >> 1 ) - 128;

  clearMatrix();
  for( uint8_t x = 0; x < PixelWidth; x++ )
  {
    if( (tick & 3) == 0 && effect.snow.pile[x] > 0 )
      effect.snow.pile[x]--;                        //melting
    uint8_t p = effect.snow.pile[x];
    if( p > 0 )
      matrix.setPixelColor( xyToNum( x, PixelHeight - 1 ), matrix.Color(p, p, p + p / 4) );
  }

  for( uint8_t i = 0; i < SnowFlakes; i++ )
  {
    effect.snow.y[i] += effect.snow.speed[i];
    if( random(256) < abs(wind) )
      effect.snow.x[i] = ( effect.snow.x[i] + (wind > 0 ? 1 : PixelWidth * 4 - 1) ) % (PixelWidth * 4);
    uint8_t x = effect.snow.x[i] / 4;
    if( effect.snow.y[i] >= (PixelHeight - 1) * 16 )
    {
      effect.snow.pile[x] = min( effect.snow.pile[x] + 10, 45 );
      snowRespawn( i, 0 );
      continue;
    }
    uint8_t b = effect.snow.speed[i] * 13;          //nearer flakes fall faster and look brighter
    matrix.setPixelColor( xyToNum( x, effect.snow.y[i] / 16 ), matrix.Color(b, b, b + b / 4) );
  }
}

void init_tickerColor_rainbow()
{
  uint8_t color_steps = 0;
  switch( colormode ) 
  {
    case 2:
       color_steps = PixelWidth;
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

/*Grey of the same luminance as c (Rec. 709 weights: green looks brightest, blue
  darkest), so it does not look brighter than c. Never quite off unless c is.*/
uint32_t greyLike( uint32_t c )
{
  uint8_t r = c >> 16, g = c >> 8, b = c;
  uint8_t l = ( 54 * (uint16_t)r + 183 * (uint16_t)g + 19 * (uint16_t)b ) >> 8;
  if( l == 0 && c != 0 )
    l = 1;
  return matrix.Color( l, l, l );
}

/*Draws the message scrolled textpos columns to the left, clipped to columns xlo..xhi*/
void tickerDraw( int16_t textpos, uint8_t xlo, uint8_t xhi )
{
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
            
            if( arguments[0] >= xlo && arguments[0] <= xhi )
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

             /*Clip both ends, the range can be narrower than a letter*/
             if( arguments[1] < xlo )
               arguments[1] = xlo;
             if( arguments[2] > xhi )
               arguments[2] = xhi;
             if( arguments[1] <= arguments[2] )
               drawhorizontalline(arguments[0], arguments[1], arguments[2], &character_without_whitespace);

             break;
            
            case 3:
              arguments[0] = letterpos + pgm_read_byte_near(&(letteraddr)[p++]) -textpos;
              
              if( arguments[0] >= xlo && arguments[0] <= xhi )
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
     if(next_pointer_toChar >= 0)
       character_without_whitespace++;
   }
   letterpos += 6;
   i++;
   }   
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

 /*Text that fits around the hat once loops over the seam between the last and the
   first column. Longer text scrolls in at the seam and out of it again, then restarts.*/
 boolean loops = pixellength <= PixelWidth;
 if( !(init & (1 << 0)) )
 {
   clearMatrix();
   if( loops )
   {
     if( ++textpos >= PixelWidth )
       textpos -= PixelWidth;
   }
   else
   {
     if (textpos == pixellength)
       textpos = -1 * (PixelWidth -1);
     textpos++;
   }
 }

 if( loops )
 {
   /*Two copies one turn apart: what scrolls past column 0 reappears at the last column*/
   int16_t turnpos = (textpos + PixelWidth) % PixelWidth;
   tickerDraw( turnpos, 0, PixelWidth - 1 );
   tickerDraw( turnpos - PixelWidth, 0, PixelWidth - 1 );
 }
 else
 {
   /*The text comes out of the seam column and flows once round the hat into it. Its
     columns start right after the seam, so they wrap past the last matrix column:
     the part up to there and the rest from column 0 on are drawn separately.*/
   uint8_t start = (TickerSeamColumn + 1) % PixelWidth;
   tickerDraw( textpos - start, start, PixelWidth - 1 );
   if( start > 0 )
     tickerDraw( textpos - start + PixelWidth, 0, start - 1 );
   /*Mark the seam, so the start of the text coming out of it can be told apart
     from the end of the text going into it right next to it. Grey contrasts with
     colored text; it is no brighter than the text color would be there (in the
     per character modes, that of the first character, which comes out of the seam).*/
   uint8_t seamx = TickerSeamColumn;
   uint8_t firstchar = 0;
   for( uint8_t y = 0; y < PixelHeight; y++ )
     matrix.setPixelColor( xyToNum( seamx, y ), greyLike( *tickerColor( &seamx, &y, &firstchar ) ) );
 }
}
