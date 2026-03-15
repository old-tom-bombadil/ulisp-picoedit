/*
  PicoEdit uLisp Extension - Version 1.1
  Hartmut Grawe - github.com/ersatzmoco - Feb 2026

  Adopted search-str from
  M5Cardputer editor version by hasn0life - Nov 2024 - https://github.com/hasn0life/ulisp-sedit-m5cardputer

  Licensed under the MIT license: https://opensource.org/licenses/MIT
*/
/*
  uLisp GIF Decode/Encode Extension v1 - 10th February 2026
  See http://www.ulisp.com/show?3IUQ
*/

// #define radiohead // Outcomment this to switch to LowPowerLab RFM69 library
// CURRENTLY MANDATORY BECAUSE OF UNSOLVED ISSUES WITHIN RADIOHEAD LIBRARY --- USE LOWPOWERLAB LIBRARY FOR NOW.

#define gif_ext
#define bmp_ext

// #define oled_gfx
#if defined(oled_gfx)
  #include <Wire.h>
  #include <U8g2lib.h>
#endif

#if defined(rfm69)
  #include <RFM69.h>
#endif

#if defined(rfm69)
    // #define FREQUENCY RF69_868MHZ
  #define FREQUENCY RF69_433MHZ
  #define ENCRYPTKEY "My@@@Encrypt@@@@" //exactly the same 16 characters/bytes on all nodes!
  #define IS_RFM69HCW true // set to 'true' only if you are using an RFM69HCW module like on Feather M0 Radio
  #define RFM69_CS 37
  #define RFM69_IRQ 36
  #define RFM69_IRQN 36
  #define RFM69_RST 35
#endif

#if defined(rfm69)
    #define PACKETLENGTH 65
    uint8_t pctlen = PACKETLENGTH+1;          //store pctlen globally for access via RadioHead library -- gets changed according to received byte packets
    char *packet = (char*)malloc(PACKETLENGTH+1);    //reserve global buffer memory for send and receive once -- PACKETLENGTH char bytes plus \0
    RFM69 radio(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
#endif

#if defined(oled_gfx)
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled = U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R2, /* reset=*/U8X8_PIN_NONE);
#endif

/*
  Helper function: Read BGR-Pixel from BMP and convert it to RGB 565 (16 bit) color value
*/
uint16_t readBGR(File file) {
  int b = file.read();
  int g = file.read();
  int r = file.read();

  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


/*
  (write-text str)
  Write string str to screen replacing DOS/cp437 character "sharp s" (225) with UTF-8 sequence for Bodmer standard font.
*/
object *fn_WriteText (object *args, object *env) {
  (void) env;

  int slength = stringlength(checkstring(first(args)))+1;
  char *tftbuf = (char*)malloc(slength);
  cstring(first(args), tftbuf, slength);

  char cs[] = {0, 0};
  char ssharp[] = {0xC3, 0xA0, 0};
  for (int i = 0; i < strlen(tftbuf); i++) {
    cs[0] = tftbuf[i];
    if (cs[0] != 225) {
      tft.print(cs);
    }
    else {
      tft.print(ssharp);
    }
  }
  //tft.print(tftbuf);
  free(tftbuf);
  return nil;
}

/*
  (write-char c)
  Write char c to screen replacing DOS/cp437 character "sharp s" (225) with UTF-8 sequence for Bodmer standard font.
*/
object *fn_WriteChar (object *args, object *env) {
  (void) env;

  char cs[] = {checkinteger(first(args)), 0};
  char ssharp[] = {0xC3, 0xA0, 0};

  if (cs[0] != 225) {
      tft.print(cs);
    }
    else {
      tft.print(ssharp);
    }

  return nil;
}


#if defined bmp_ext
/*
  (display-bmp fname x y)
  Open BMP file fname from SD if it exits and display it on screen at position x y.
*/
object *fn_DisplayBMP (object *args, object *env) {
  (void) env;

  SD.begin(SDCARD_SS_PIN);

  int slength = stringlength(checkstring(first(args)))+1;
  char *fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);
  File file;

  if (!SD.exists(fnbuf)) {
    pfstring("File not found", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  int x = checkinteger(second(args));
  int y = checkinteger(third(args));
  (void) args;

  char buffer[BUFFERSIZE];
  file = SD.open(fnbuf);
  if (!file) { 
    pfstring("Problem reading from SD card", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  char b = file.read();
  char m = file.read();
  if ((m != 77) || (b != 66)) {
    pfstring("No BMP file", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  file.seek(10);
  uint32_t offset = SDRead32(file);
  SDRead32(file);
  int32_t width = SDRead32(file);
  int32_t height = SDRead32(file);
  int zpad = 0;
  if ((width % 4) > 0) {
    zpad = (4 - ((width * 3) % 4));
  }

  file.seek(offset);

  uint16_t *linebuf  = (uint16_t*)malloc(width * sizeof(uint16_t));
  uint8_t  *sdbuf    = (uint8_t*)malloc(width * 3);

  tft.startWrite();
  for (int ly = (y + height - 1); ly >= y; ly--) {

      file.read(sdbuf, width * 3);

      // convert pixel line to RGB565
      for (int lx = 0; lx < width; lx++) {
          uint8_t b = sdbuf[lx*3];
          uint8_t g = sdbuf[lx*3 + 1];
          uint8_t r = sdbuf[lx*3 + 2];
          linebuf[lx] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
      }

      for (int i = 0; i < zpad; i++) file.read();

      tft.setAddrWindow(x, ly, width, 1);
      tft.pushColors(linebuf, width, true);
  }
  tft.endWrite();

  free(linebuf);
  free(sdbuf);

  file.close();
  free(fnbuf);
  return nil;
}

/*
  (load-bmp fname arr [offx] [offy])
  Open RGB BMP file fname from SD if it exits and copy it into the two-dimensional uLisp array provided.
  Note that this allocates massive amounts of RAM; use for small bitmaps/icons only.
  When the image is larger than the array, only the upper leftmost area of the bitmap fitting into the array is loaded.
  Providing offx and offy you may move the "window" of the array to other parts of the bitmap (useful e.g. for tiling).
*/
object *fn_LoadBMP (object *args, object *env) {

  SD.begin(SDCARD_SS_PIN);

  int slength = stringlength(checkstring(first(args)))+1;
  char* fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);
  File file;

  if (!SD.exists(fnbuf)) {
    pfstring("File not found", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  object* array = second(args);
  if (!arrayp(array)) {
    pfstring("Argument is not an array", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  object* dimensions = cddr(array);
  if (listp(dimensions)) {
    if (listlength(dimensions) != 2) {
      pfstring("Array must be two-dimensional", (pfun_t)pserial);
      free(fnbuf);
      return nil;
    }
  }
  else {
    pfstring("Array must be two-dimensional", (pfun_t)pserial);
      free(fnbuf);
      return nil;
  }
  args = cddr(args);
  int offx = 0;
  int offy = 0;

  if (args != NULL) {
    offx = checkinteger(car(args));
    args = cdr(args);
    if (args != NULL) {
      offy = checkinteger(car(args));
    }
  }
  
  (void) args;

  int aw = first(dimensions)->integer;
  int ah = second(dimensions)->integer;
  int bit;
  object* subscripts;
  object* ox;
  object* oy;
  object* oyy;
  object** element;

  char buffer[BUFFERSIZE];
  file = SD.open(fnbuf);
  if (!file) { 
    pfstring("Problem reading from SD card", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  char b = file.read();
  char m = file.read();
  if ((m != 77) || (b != 66)) {
    pfstring("No BMP file", (pfun_t)pserial);
    free(fnbuf);
    file.close();
    return nil;
  }

  file.seek(10);
  uint32_t offset = SDRead32(file);
  SDRead32(file);
  int32_t width = SDRead32(file);
  int32_t height = SDRead32(file);
  int zpad = 0;
  if ((width % 4) > 0) {
    zpad = (4 - ((width * 3) % 4));
  }

  file.seek(offset);

  for (int ly = (height - 1); ly >= 0; ly--) {
    for (int lx = 0; lx < width; lx++) {
      if ((lx < (aw+offx)) && (ly < (ah+offy)) && (lx >= offx) && (ly >= offy)) {
        ox = number(lx-offx);
        oy = number(ly-offy);
        oyy = cons(oy, NULL);
        subscripts = cons(ox, oyy);
        element = getarray(array, subscripts, env, &bit);
        *element = number(readBGR(file));
      }
      else {
        file.read();
        file.read();
        file.read();
      }
    }
    //ignore trailing zero bytes
    if (zpad > 0) {
      for (int i = 0; i < zpad; i++) {
        file.read();
      }
    }
  }

  file.close();
  free(fnbuf);
  return nil;
}

/*
  (load-mono fname arr [offx] [offy])
  Open monochrome BMP file fname from SD if it exits and copy it into the two-dimensional uLisp bit array provided.
  When the image is larger than the array, only the upper leftmost area of the bitmap fitting into the array is loaded.
  Providing offx and offy you may move the "window" of the array to other parts of the bitmap (useful e.g. for tiling).
*/
object *fn_LoadMono (object *args, object *env) {

  SD.begin(SDCARD_SS_PIN);

  int slength = stringlength(checkstring(first(args)))+1;
  char* fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);
  File file;

  if (!SD.exists(fnbuf)) {
    pfstring("File not found", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  object* array = second(args);
  if (!arrayp(array)) {
    pfstring("Argument is not an array", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  object* dimensions = cddr(array);
  if (listp(dimensions)) {
    if (listlength(dimensions) != 2) {
      pfstring("Array must be two-dimensional", (pfun_t)pserial);
      free(fnbuf);
      return nil;
    }
  }
  else {
    pfstring("Array must be two-dimensional", (pfun_t)pserial);
      free(fnbuf);
      return nil;
  }
  args = cddr(args);
  int offx = 0;
  int offy = 0;

  if (args != NULL) {
    offx = checkinteger(car(args));
    args = cdr(args);
    if (args != NULL) {
      offy = checkinteger(car(args));
    }
  }
  (void) args;

  int aw = abs(first(dimensions)->integer);
  int ah = abs(second(dimensions)->integer);
  int bit;
  object* subscripts;
  object* ox;
  object* oy;
  object* oyy;
  object** element;

  char buffer[BUFFERSIZE];
  file = SD.open(fnbuf);
  if (!file) { 
    pfstring("Problem reading from SD card", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  char b = file.read();
  char m = file.read();
  if ((m != 77) || (b != 66)) {
    pfstring("No BMP file", (pfun_t)pserial);
    free(fnbuf);
    file.close();
    return nil;
  }

  file.seek(10);
  uint32_t offset = SDRead32(file);
  SDRead32(file);
  int32_t width = SDRead32(file);
  int32_t height = SDRead32(file);
  int linebytes = width / 8;
  int restbits = width % 8;
  if (restbits > 0) linebytes++;
  int zpad = 0;
  if ((linebytes % 4) > 0) {
    zpad = (4 - (linebytes % 4));
  }

  file.seek(28);
  uint16_t depth = file.read();
  if (depth > 1) { 
    pfstring("No monochrome bitmap file", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  file.seek(offset);

  int lx = 0;
  int bmpbyte = 0;
  int bmpbit = 0;

  for (int ly = (height - 1); ly >= 0; ly--) {
    for (int bx = 0; bx < linebytes; bx++) {
      bmpbyte = file.read();
      for (int bix = 0; bix < 8; bix++) {
        lx = (bx * 8) + bix;
        if ((lx < (aw+offx)) && (ly < (ah+offy)) && (lx >= offx) && (ly >= offy)) {
          ox = number(lx-offx);
          oy = number(ly-offy);
          oyy = cons(oy, NULL);
          subscripts = cons(ox, oyy);
          element = getarray(array, subscripts, env, &bit);

          bmpbit = bmpbyte & (1 << (7-bix));
          if (bmpbit > 0) {
            bmpbit = 1;
          }
          else {
            bmpbit = 0;
          }
          *element = number((checkinteger(*element) & ~(1u<<bit)) | bmpbit<<bit);
        }
      }
    }
    //ignore trailing zero bytes
    if (zpad > 0) {
      for (int i = 0; i < zpad; i++) {
        file.read();
      }
    }
  }

  file.close();
  free(fnbuf);
  return nil;
}

/*
  (show-bmp arr x y [monocol])
  Show bitmap image contained in uLisp array arr on screen at position x y.
  The function automatically distinguishes between monochrome and color image arrays.
  If monocol is provided, a monochrome image is painted with that color value.
*/
object *fn_ShowBMP (object *args, object *env) {

  object* array = first(args);
  if (!arrayp(array)) error2("argument is not an array");

  object *dimensions = cddr(array);
  if (listp(dimensions)) {
    if (listlength(dimensions) != 2) error2("array must be two-dimensional");
  }
  else error2("array must be two-dimensional");

  int x = checkinteger(second(args));
  int y = checkinteger(third(args));

  int monocol = 0xFFFF;
  args = cddr(args);
  args = cdr(args);
  if (args != NULL) {
    monocol = checkinteger(first(args));
  }
  (void) args;

  int aw = abs(first(dimensions)->integer);
  int ah = abs(second(dimensions)->integer);
  int bit;
  object* subscripts;
  object* ox;
  object* oy;
  object* oyy;
  object** element;

  tft.startWrite();
  int starttime;
  int bmpbit = 0;
  uint16_t color = 0;
  for (int ay = 0; ay < ah; ay++) {
    for (int ax = 0; ax < aw; ax++) {
      ox = number(ax);
      oy = number(ay);
      oyy = cons(oy, NULL);
      subscripts = cons(ox, oyy);
      element = getarray(array, subscripts, env, &bit);
      if (bit < 0) {
        tft.drawPixel(x+ax, y+ay, checkinteger(*element));
      }
      else {
        bmpbit = abs(checkinteger(*element) & (1<<bit));
        if (bmpbit > 0) {
          color = monocol;
        }
        else {
          color = 0;
        }
        tft.drawPixel(x+ax, y+ay, color);
      }
    }
  }
  tft.endWrite();

  return nil;
}
#endif

/*
  GIF extension by David Johnson-Davies
*/
#if defined gif_ext
typedef struct {
  int16_t ptr;
  uint8_t code;
} cell_t;

uint16_t Colour (int r, int g, int b) {
  return (r & 0xf8)<<8 | (g & 0xfc)<<3 | b>>3;
}

// Globals
const int TableSize = 4096;
cell_t Table[TableSize]; // Translation table
uint8_t Block[255]; 
uint16_t ColourTable[256];
int Pixel = 0;

// Utilities **********************************************

/*
  InitialiseTable - initialises the translation table.
*/
void InitialiseTable (int colours) {
  for (int c = 0; c<colours; c++) {
    Table[c].ptr = -1; Table[c].code = c;
  }
}

/*
  IntegerLength -  number of bits needed to represent n
*/
int IntegerLength (unsigned int n) {
  return n ? 32 - __builtin_clz(n) : 0;
}

// Display GIF **********************************************

/*
  GetNBits - reads the next n bits from the bitstream. It keeps a bit buffer in the variable buf, with nbuf
  specifying how many bits are still available. If there are fewer than n bits in buf another byte is read
  from the input stream and appended to the buffer.
*/
int GetNBits (gfun_t gfun, int n) {
  static uint8_t nbuf = 0, blockptr = 0;
  static uint32_t buf = 0;
  if (n > 0) {
    while (nbuf < n) {
      if (blockptr == 0) blockptr = gfun();
      buf = ((uint32_t)gfun() << nbuf) | buf;
      blockptr--; nbuf = nbuf + 8;
    }
    int result = ((1 << n) - 1) & buf;
    buf = buf >> n; nbuf = nbuf - n;
    return result;
  } else { // n == 0
    nbuf = 0; buf = 0; blockptr = 0;
    return 0;
  }
}

/*
  FirstPixel - for a table element p, follows the pointers to find the first pixel colour in the sequence it represents.
*/
uint8_t FirstPixel (int p) {
  uint8_t last;
  do {
    last = Table[p].code;
    p = Table[p].ptr;
  } while (p != -1);
  return last;
}

/*
  WritePixel - calls a Lisp function with parameters x, y, and c to plot a pixel.
*/
void WritePixel (object *fn, uint16_t x, uint16_t y, uint16_t c) {
  if (fn == NULL) {
    #if defined(gfxsupport)
      tft.drawPixel(x, y, c);
    #endif
  } else {
    object* list = cons(number(x), cons(number(y), (cons(number(c), NULL))));
    apply(fn, list, NULL);
  }
}

/*
  PlotSequence - plots the sequence of pixels represented by the table element p. It avoids the need for recursion by
  plotting pixels backwards.
*/
void PlotSequence (object *fn, int p, uint16_t width) {
  // Measure backtrack
  int i = 0; int16_t rest = p;
  while (rest != -1) {
    rest = Table[rest].ptr;
    i++;
  }
  // Plot backwards
  Pixel = Pixel + i - 1;
  rest = p;
  while (rest != -1) {
    WritePixel(fn, Pixel%width, Pixel/width, ColourTable[Table[rest].code]);
    Pixel--;
    rest = Table[rest].ptr;
  }
  Pixel = Pixel + i + 1;
  #if defined(ARDUINO_M5STACK_TAB5)
  if (!fn) tft.display();
  #endif
}

/*
  SkipNBytes - skips a series of unneeded bytes in the input stream.
*/
void SkipNBytes (gfun_t gfun, int n) {
  for (int i=0; i<n; i++) gfun();
}

/*
  Power2 - checks that its argument is a power of two. It is used to increase the bit length of codes
  being read when the size of the table reaches a power of two.
*/
boolean Power2 (int x) {
  return (x & (x - 1)) == 0;
}

int ReadInt (gfun_t gfun) {
  return gfun() | gfun()<<8;
}

/*
  (decode-gif stream [function])
  Decodes a GIF file from the stream and plots it.
  Alternatively a function fn(x y colours) can be provided that plots the colour index at x,y.
*/
object *fn_DecodeGIF (object *args, object *env) {
  (void) env;
  gfun_t gfun = gstreamfun(args);
  args = cdr(args);
  object *fn = NULL;
  if (args != NULL) fn = first(args);
  const char *head = "GIF8*a"; // GIF87a or GIF89a
  for (int p=0; head[p]; p++) {
    if (gfun() != head[p] && head[p] != '*') error2("not a valid GIF");
  }
  int width = ReadInt(gfun);
  ReadInt(gfun); // Height
  uint8_t field = gfun();
  SkipNBytes(gfun, 2); // background, and aspect
  uint8_t colourbits = max(1 + (field & 7), 2);
  int colours = 1<<colourbits;
  int clr = colours;
  int end = 1 + clr;
  int free = 1 + end;
  uint8_t bits = 1 + colourbits;
  Pixel = 0;

  // Parse colour table
  for (int c = 0; c<colours; c++) {
    ColourTable[c] = Colour(gfun(), gfun(), gfun());
  }
  
  InitialiseTable(colours);
  
  // Parse blocks
  do {
    uint8_t header = gfun();
    if (header == 0x2C) { // Image block
      SkipNBytes(gfun, 8);
      uint8_t packed = gfun();
      if (packed & 0x80) error2("local colour tables not supported");
      if (packed & 0x40) error2("interlaced GIFs not supported");
      SkipNBytes(gfun, 1);
      GetNBits(gfun, 0); // Reset nbuf, buf, and blockptr
      int code = -1, last = -1;
      do {
        last = code;
        code = GetNBits(gfun, bits);
        if (code == clr) {
          free = 1 + end;
          bits = 1 + colourbits;
          code = -1;
        } else if (code == end) {
          break;
        } else if (last == -1) {
          PlotSequence(fn, code, width);
        } else if (code <= free) {
          if (free < TableSize) {
            Table[free].ptr = last;
            if (code == free) Table[free].code = FirstPixel(last);
            else Table[free].code = FirstPixel(code);
            free++;
            if (Power2(free) && (free < TableSize)) bits++;
          }
          PlotSequence(fn, code, width); 
        }
      } while (true);
      if (gfun() != 0) error2("invalid end of file");
    } else if (header == 0x21) { // Extension block
      uint8_t gce = gfun();
      if (gce == 0xff) { // Application extension
        SkipNBytes(gfun, 1);
        SkipNBytes(gfun, 8);
        const char *xmp = "XMP"; // Adobe extension
        for (int p=0; xmp[p]; p++) {
          if (gfun() != xmp[p]) error2("unrecognised application extension");
        }
        int length;
        do {
          length = gfun();
          SkipNBytes(gfun, length);
        } while (length != 0);
      } else { // Other extension
        int length = gfun();
        SkipNBytes(gfun, 1 + length);
      }
    } else if (header == 0x3b) { // Terminating byte
      return nil;
    }
  } while (true);
  return nil;
}

// Encode GIF **********************************************

// Works: 8, 64, 256

const int factor=0, divisor = 1, rescale = 2;

const uint8_t PosteriseData [8][3][3] {
  //factors    divisors   rescales
  { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },             // 2 not used
  { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} },             // 4 not used
  { {2, 2, 2}, {16, 32, 16}, {0x80, 0x80, 0x80} }, // 8
  { {2, 3, 2}, {16, 22, 16}, {0x80, 0xb0, 0x80} }, // 16
  { {3, 3, 3}, {11, 22, 11}, {0xb0, 0xb0, 0xb0} }, // 32
  { {4, 4, 4}, {8, 16, 8}, {0xc0, 0xc0, 0xc0} },   // 64
  { {5, 5, 5}, {7, 13, 7}, {0xe0, 0xd0, 0xe0} },   // 128
  { {6, 7, 6}, {6, 10, 6}, {0xf0, 0xf0, 0xf0} }    // 256
};

/*
  EmitTable - writes out the colour table for the specified number of colours.
*/
void EmitTable (pfun_t pfun, int colours) {
  uint8_t bits = IntegerLength(colours - 1);
  const uint8_t (*entry)[3] = PosteriseData[bits - 1];
  uint8_t product = entry[factor][0] * entry[factor][1] * entry[factor][2];
  if (colours == 2) {
    pfun(0); pfun(0); pfun(0); pfun(0xff); pfun(0xff); pfun(0xff);
  } else if (colours == 4) {
    pfun(0); pfun(0); pfun(0); pfun(0); pfun(0xff); pfun(0); 
    pfun(0xff); pfun(0); pfun(0); pfun(0xff); pfun(0xff); pfun(0xff);
  } else {
    for (int r=0; r<entry[factor][0]; r++) {
      for (int g=0; g<entry[factor][1]; g++) {
        for (int b=0; b<entry[factor][2]; b++) {
          pfun((((r * entry[divisor][0]) << 3) * 0xff) / entry[rescale][0]);
          pfun((((g * entry[divisor][1]) << 2) * 0xff) / entry[rescale][1]);
          pfun((((b * entry[divisor][2]) << 3) * 0xff) / entry[rescale][2]);
        }
      }
    }
    for (int w=0; w<colours - product; w++) { pfun(0); pfun(0); pfun(0); }
  }
}

/*
  Posterise - converts a 16-bit colour col565 to an 8-bit colour index in the 
  specified number of colours by posterising it, and returns it.
*/
uint8_t Posterise (uint16_t col565, int colours) {
  uint8_t bits = IntegerLength(colours - 1);
  const uint8_t (*entry)[3] = PosteriseData[bits - 1];
  uint8_t r = col565>>11 & 0x1f, g = col565>>5 & 0x3f, b = col565 & 0x1f;
  if (colours == 2) {
    if (col565 == 0) return 0; else return 1;
  } else if (colours == 4) {
    if (col565 == 0) return 0; else if (r>0 && g>0 && b>0) return 3; else if (r>0) return 2; else return 1;
  } else {
    return (r / entry[divisor][0]) * entry[factor][1] * entry[factor][2] +
    (g / entry[divisor][1]) * entry[factor][2] + (b / entry[divisor][2]);
  }
}

/*
  (posterise col565 colours)
  Converts a 16-bit colour col565 to an 8-bit colour index in the 
  specified number of colours by posterising it, and returns it.
*/
object *fn_Posterise (object *args, object *env) {
  (void) env;
  int colours = checkinteger(second(args));
  if (colours < 0 || colours > 256) error("invalid number of colours", second(args));
  return number(Posterise(checkinteger(first(args)), colours));
}

/*
  PutNBits - adds n bits from the bitstring 'bits' to the bitstream in 'buf', writing blocks to the output stream when full.
  If n=0 resets the static variables. If n=-1 flushes the bitstream.
*/
void PutNBits (pfun_t pfun, int n, int bits) {
  static uint8_t nbuf = 0;     // Number of bits in buf
  static uint8_t blockptr = 0; // Index in Block[] array
  static uint32_t buf = 0;     // Bit buffer
  if (n > 0) {
    buf = bits<<nbuf | buf;
    nbuf = nbuf + n;
    while (nbuf >= 8) {
      Block[blockptr++] = buf & 0xff;
      buf = buf >> 8; nbuf = nbuf - 8;
      if (blockptr == 255) {
        pfun(blockptr);
        for (uint8_t i=0; i<blockptr; i++) pfun(Block[i]);
        blockptr = 0;
      }
    }
    return;
  } else if (n == -1) { // flush bitstream
    if (nbuf > 0) {
      Block[blockptr++] = buf & 0xff;
    }
    if (blockptr != 0) {
      pfun(blockptr);
      for (uint8_t i=0; i<blockptr; i++) pfun(Block[i]);
    }
    pfun(0); // terminator
  } else { // n == 0
    nbuf = 0; buf = 0; blockptr = 0;
  }
}

/*
  GetPixel - reads the display and returns a colour index for the point x,y for the specified number of colours.
*/
int GetPixel (object *fn, int start, int length, int xsize, int colours) {
  if (start < length) {
    if (fn == NULL) {
      #if defined(ARDUINO_WIO_TERMINAL) || defined(ARDUINO_M5STACK_TAB5) || defined(PICOCALC)
      uint16_t col565 = tft.readPixel(start%xsize, start/xsize);
      #else
      uint16_t col565 = readpixel(start%xsize, start/xsize);
      #endif
      return Posterise(col565, colours);
    } else {
      object *list = cons(number(start%xsize), cons(number(start/xsize), (cons(number(colours), NULL))));
      object *result = apply(fn, list, NULL);
      int index = checkinteger(result);
      if (index < 0 || index >= colours) error("colour index out of range", result);
      return index;
    }
  } else {
    return -1;
  }
}

/*
  (encode-gif stream xsize ysize colours [function] [noclear])
  Encodes a GIF file by reading the display, and outputs the encoding with the specified number of colours to the stream.
  Alternatively a function fn(x y colours) can be provided; it is called for each point, and should return the colour index.
  Setting noclear to t can give a smaller file size for images with repetitive content.
*/
object *fn_EncodeGIF (object *args, object *env) {
  (void) env;
  pfun_t pfun = pstreamfun(args);
  args = cdr(args);
  int xsize = checkinteger(first(args));
  int ysize = checkinteger(second(args));
  int colours = checkinteger(third(args));
  bool doclear = true;
  args = cdr(cddr(args));
  object *function = NULL;
  if (args != NULL) {
    function = first(args);
    args = cdr(args);
    if (args != NULL && first(args) != NULL) doclear = false;
  }
  int start = 0;
  int length = xsize * ysize;
  if (colours < 0 || colours > 256) error("invalid number of colours", third(args));
  uint8_t colourbits = IntegerLength(colours - 1);
  uint8_t codesize = colourbits<2 ? 2 : colourbits;
  int clr = 1<<codesize;
  int end = clr + 1;
  int free = end + 1;
  PutNBits(pfun, 0, 0); // clear nbuf, buf, and blockptr

  // Header
  const char *head = "GIF87a";
  for (int p=0; head[p]; p++) pfun(head[p]);

  // Screen descriptor
  pfun(xsize & 0xff); pfun(xsize >> 8); // Image width
  pfun(ysize & 0xff); pfun(ysize >> 8); // Image height
  pfun(0x80 | (colourbits - 1)<<4 | 0x00 | (colourbits - 1)); // Packed fields
  pfun(0); // Colour 0 = background
  pfun(0); // No odd aspect ratio

  // Global colour table
  EmitTable(pfun, colours);

  // Image descriptor
  pfun(0x2c);
  pfun(0); pfun(0); // Image top
  pfun(0); pfun(0); // Image left
  pfun(xsize & 0xff); pfun(xsize >> 8); // Image width
  pfun(ysize & 0xff); pfun(ysize >> 8); // Image height
  pfun(0); // Packed fields

  // Image data
  InitialiseTable(colours);
  pfun(codesize);
  PutNBits(pfun, IntegerLength(free - 1), clr);
  do {
    // Look up a sequence in translation table
    int ptr = GetPixel(function, start, length, xsize, colours);
    int second = GetPixel(function, start+1, length, xsize, colours);
    if (second == -1) {
      PutNBits(pfun, IntegerLength(free - 1), ptr);
    } else {
      int x = end + 1;
      while ((x != free) && (second != -1)) {
        if ((Table[x].ptr == ptr) && (Table[x].code == second)) {
          ptr = x;
          start++;
          second = GetPixel(function, start+1, length, xsize, colours);
        }
        x++;
      }
      PutNBits(pfun, IntegerLength(free - 1), ptr);
      if (free < TableSize) {
        Table[free].ptr = ptr;
        Table[free].code = GetPixel(function, start+1, length, xsize, colours);
        free++;
      }
    }
    start++;
    if ((free == TableSize) && doclear) {
      PutNBits(pfun, IntegerLength(free - 1), clr);
      free = end + 1;
    }
  } while (start < length);
  PutNBits(pfun, IntegerLength(free - 1), end);
  PutNBits(pfun, -1, 0); // Flush bitstream
  pfun(0x3b);
  return nil;
}
#endif

/*
  Helper function:
  Translate keys if necessary - separate from REPL key translation
*/
char translate_key (uint16_t temp, uint8_t mod) {
    unsigned char kout = 0;
    //Serial.print(mod); Serial.print(" "); Serial.println(temp);
    switch (temp) {
      case 96: kout = '^'; break;
      case 93: 
          if (mod == 0) {
              kout = '+';
          }
          else if (mod == 0x40) {
              kout = '~';
          }
          break;

      case 121: kout = 'z'; break;
      case 122: kout = 'y'; break;
      case 89: kout = 'Z'; break;
      case 90: kout = 'Y'; break;
      case 47: kout = '-'; break;
      case 92: kout = '#'; break;
      case 61: kout = '|'; break;

      case 39: kout = '('; break;
      case 91: kout = ')'; break;
      case 59: kout = '['; break;

      case 64:
          if (mod == 2) kout = '\"'; break;
  //    case 35:
  //        if (mod == 2) kout = 0xA7; break;
      case 94:
          if (mod == 2) kout = '&'; break;
      case 38:
          if (mod == 2) kout = '/'; break;
      case 42:
          if (mod == 2) kout = '('; break;
      case 40:
          if (mod == 2) kout = ')'; break;
      case 41:
          if (mod == 2) kout = '='; break;

      case 125:
          if (mod == 2) kout = '*'; break;

      case 60:
          if (mod == 2) kout = ';'; break;
      case 62:
          if (mod == 2) kout = ':'; break;
      case 63:
          if (mod == 2) kout = '_'; break;
      case 124:
          if (mod == 2) kout = '\''; break;
      case 95:
          if (mod == 2) kout = '?'; break;
      case 34:
          if (mod == 2) kout = '<'; break;
      case 123:
          if (mod == 2) kout = '>'; break;
      case 58:
          if (mod == 2) kout = ']'; break;

      case 55:
          if (mod == 0x40) {
            kout = '{';
          }
          else if (mod == 4) {
            kout = 247;
          }
          else {
            kout = temp;
          } 
          break;
      case 56:
          if (mod == 0x40) {
            kout = '[';
          }
          else if (mod == 4) {
            kout = 248;
          }
          else {
            kout = temp;
          } 
          break;
      case 57:
          if (mod == 0x40) {
            kout = ']';
          }
          else if (mod == 4) {
            kout = 249;
          }
          else {
            kout = temp;
          } 
          break;
      case 48:
          if (mod == 0x40) {
            kout = '}';
          }
          else if (mod == 4) {
            kout = 240;
          }
          else {
            kout = temp;
          } 
          break;

      case 113:
          if (mod == 0x40) {
            kout = '@';
          }
          else {
            kout = temp;
          }
          break;

      case 45:
          if (mod == 0x40) kout = '\\'; break;

      case 99:
          if (mod == 4) {
            kout = 251;
          }
          else {
            kout = temp;
          } 
          break;

      case 118:
          if (mod == 4) {
            kout = 252;
          }
          else {
            kout = temp;
          } 
          break;

      case 120:
          if (mod == 4) {
            kout = 253;
            }
          else {
            kout = temp;
          } 
          break;

      case 49:
          if (mod == 4) {
            kout = 241;
          }
          else {
            kout = temp;
          } 
          break;

      case 50:
          if (mod == 4) {
            kout = 242;
          }
          else {
            kout = temp;
          } 
          break;

      case 51:
          if (mod == 4) {
            kout = 243;
          }
          else {
            kout = temp;
          }
          break;

      case 52:
          if (mod == 4) {
            kout = 244;
          }
          else {
            kout = temp;
          } 
          break;

      case 53:
          if (mod == 4) {
            kout = 245;
          }
          else {
            kout = temp;
          }
          break;

      case 54:
          if (mod == 4) {
            kout = 246;
          }
          else {
            kout = temp;
          } 
          break;

      default: kout = temp; break;
    }

  return kout;
}
 
/*
  (keyboard-get-key [translate] [state])
  Waits for a key press and returns its ASCII value.
  If translate is t the ASCII value is translated with function translate_key.
  If state = 1, 2 or 3 (pressed, long-pressed, released) return key only if it was recognized with requested state .
*/
object *fn_KeyboardGetKey (object *args, object *env) {
  (void) env;

  bool translate = false;
  int reqstate = PCKeyboard::StatePress;

  if (args != NULL) {
    translate = (first(args) == nil) ? false : true;
    args = cdr(args);
  }
  if (args != NULL) {
    int mystate = checkinteger(first(args));
    reqstate = constrain(mystate, 1, 3);
  }

  for (;;) {
    testescape();
    if (pc_kbd.keyCount() > 0) {
      const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
      if (key.state == reqstate) {
        char temp = key.key;

        if ((temp > 0) && (temp < 255)) {
          if (translate) {
            return number(translate_key(temp, 0));
          }
          else {
            return number(temp);
          }
        }
      }
    }
  }
}

/*
  (keyboard-key-pressed [translate])
  Returns key number and state as list if a key was pressed, nil if there is no keyboard data.
  If translate is t the ASCII value is translated with function translate_key.
*/
object *fn_KeyboardKeyPressed (object *args, object *env) {
  (void) env;

  bool translate = false;
  int state;

  if (args != NULL) {
    translate = (first(args) == nil) ? false : true;
  }

  if (pc_kbd.keyCount() > 0) {
      const PCKeyboard::KeyEvent key = pc_kbd.keyEvent();
      state = key.state;
      char temp = key.key;

      if ((temp > 0) && (temp < 255)) {
        if (translate) {
          return cons(number(translate_key(temp, 0)), cons(number(state), nil));
        }
        else {
          return cons(number(temp), cons(number(state), nil));
        }
      }
      else {
        return nil;
      }
  }
  else {
      return nil;
  }
}


/*
  (search-str pattern target [startpos])
  Returns the index of the first occurrence of pattern in target, or nil if it's not found starting from startpos.
*/
object *fn_searchstr (object *args, object *env) {
  (void) env;
  
  int startpos = 0;
  object *pattern = first(args);
  object *target = second(args);
  args = cddr(args);
  if (pattern == NULL) return number(0);
  else if (target == NULL) return nil;
  if (args != NULL) startpos = checkinteger(car(args));
  
if (stringp(pattern) && stringp(target)) {
    int l = stringlength(target);
    int m = stringlength(pattern);
    if (startpos > l) error2(indexrange);
    for (int i = startpos; i <= l-m; i++) {
      int j = 0;
      while (j < m && nthchar(target, i+j) == nthchar(pattern, j)) j++;
      if (j == m) return number(i);
    }
    return nil;
  } else error2("arguments are not both lists or strings");
  return nil;
}

/*
  (rad-to-deg n)
  Convert radians to degrees.
*/
object *fn_RadToDeg (object *args, object *env) {
  (void) env;

  return makefloat(checkintfloat(first(args))*RAD_TO_DEG);
}

/*
  (deg-to-rad n)
  Convert degree to radians.
*/
object *fn_DegToRad (object *args, object *env) {
  (void) env;

  return makefloat(checkintfloat(first(args))*DEG_TO_RAD);
}

/*
  (vector-sub v1 v2)
  Subtract vector v2 from vector v1 (two lists of number elements).
*/
object *fn_VectorSub (object *args, object *env) {
  (void) env;

  object *v1 = first(args);
  object *v2 = second(args);
  if (!listp(v1) || !listp(v2)) error2("arguments must be two lists of numbers");

  object *retlist = NULL;
  float a, b = 0;

  while ((v1 != NULL) && (v2 != NULL)) {
    a = checkintfloat(car(v1));
    b = checkintfloat(car(v2));
    retlist = cons(makefloat(a-b), retlist);
    v1 = cdr(v1);
    v2 = cdr(v2);
  }

  return fn_reverse(cons(retlist, NULL), NULL);
}

/*
  (vector-add v1 v2)
  Add vector v2 to vector v1 (two lists of number elements).
*/
object *fn_VectorAdd (object *args, object *env) {
  (void) env;

  object *v1 = first(args);
  object *v2 = second(args);
  if (!listp(v1) || !listp(v2)) error2("arguments must be two lists of numbers");

  object *retlist = NULL;
  float a, b = 0;

  while ((v1 != NULL) && (v2 != NULL)) {
    a = checkintfloat(car(v1));
    b = checkintfloat(car(v2));
    retlist = cons(makefloat(a+b), retlist);
    v1 = cdr(v1);
    v2 = cdr(v2);
  }

  return fn_reverse(cons(retlist, NULL), NULL);
}

/*
  (vector-norm v)
  Calculate magnitude/norm of vector v (list of number elements).
*/
object *fn_VectorNorm (object *args, object *env) {
  (void) env;

  object *v1 = first(args);
  if (!listp(v1)) error2("argument must be a list");

  float a, sum = 0;
  while (v1 != NULL) {
    a = checkintfloat(car(v1));
    sum = sum + (a*a);
    v1 = cdr(v1);
  }  

  return makefloat(sqrt(sum));
}

/*
  (scalar-mult v s)
  Multiply vector v (list of number elements) by number s (scalar).
*/
object *fn_ScalarMult (object *args, object *env) {
  (void) env;

  object *v1 = first(args);
  if (!listp(v1)) error2("first argument must be a list");
  float s = checkintfloat(second(args));

  object *retlist = NULL;
  float a;
  while (v1 != NULL) {
    a = checkintfloat(car(v1));
    retlist = cons(makefloat(a*s), retlist);
    v1 = cdr(v1);
  }

  return fn_reverse(cons(retlist, NULL), NULL);
}

/*
  (dot-product v1 v2)
  Calculate dot product of two vectors v1, v2 (lists of number elements).
*/
object *fn_DotProduct (object *args, object *env) {
  (void) env;

  object *v1 = first(args);
  object *v2 = second(args);
  if (!listp(v1) || !listp(v2)) error2("arguments must be two lists of numbers");

  float a, b;
  float sum = 0;

  while ((v1 != NULL) && (v2 != NULL)) {
    a = checkintfloat(car(v1));
    b = checkintfloat(car(v2));
    sum = sum + a*b;
    v1 = cdr(v1);
    v2 = cdr(v2);
  }

  return makefloat(sum);
}

/*
  (cross-product v1 v2)
  Calculate cross product of two three-dimensional vectors v1, v2 (lists with 3 elements).
*/
object *fn_CrossProduct (object *args, object *env) {
  (void) env;

  if (!listp(first(args)) || !listp(second(args))) error2("arguments must be lists of three numbers");

  float a1 = checkintfloat(car(first(args)));
  float a2 = checkintfloat(car(cdr(first(args))));
  float a3 = checkintfloat(car(cddr(first(args))));

  float b1 = checkintfloat(car(second(args)));
  float b2 = checkintfloat(car(cdr(second(args))));
  float b3 = checkintfloat(car(cddr(second(args))));

  float c1 = a2*b3 - a3*b2;
  float c2 = a3*b1 - a1*b3;
  float c3 = a1*b2 - a2*b1;

  return cons(makefloat(c1), cons(makefloat(c2), cons(makefloat(c3), NULL)));
}

/*
  (vector-angle v1 v2)
  Calculate angle (rad) between two three-dimensional vectors v1, v2 (lists with 3 elements).
*/
object *fn_VectorAngle (object *args, object *env) {
  (void) env;

  if (!listp(first(args)) || !listp(second(args))) error2("arguments must be lists of three numbers");

  float a1 = checkintfloat(car(first(args)));
  float a2 = checkintfloat(car(cdr(first(args))));
  float a3 = checkintfloat(car(cddr(first(args))));

  float b1 = checkintfloat(car(second(args)));
  float b2 = checkintfloat(car(cdr(second(args))));
  float b3 = checkintfloat(car(cddr(second(args))));

  //dot product
  double dot = (a1*b1 + a2*b2 + a3*b3);

  //norms
  double na = sqrt(a1*a1 + a2*a2 + a3*a3);
  double nb = sqrt(b1*b1 + b2*b2 + b3*b3);

  float cphi = dot/(na*nb);

  return makefloat(acos(cphi));
}


#if defined sdcardsupport
/*
  (sd-file-exists filename)
  Returns t if filename exists on SD card, otherwise nil.
*/
object *fn_SDFileExists (object *args, object *env) {
  (void) args, (void) env;

  SD.begin(SDCARD_SS_PIN);

  int slength = stringlength(checkstring(first(args)))+1;
  char *fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);

  if (SD.exists(fnbuf)) {
    free(fnbuf);
    return tee;
  }
  else {
    free(fnbuf);
    return nil;
  }
}

/*
  (sd-file-remove filename)
  Returns t if filename exists on SD card, otherwise nil.
*/
object *fn_SDFileRemove (object *args, object *env) {
  (void) args, (void) env;

  SD.begin(SDCARD_SS_PIN);
  int slength = stringlength(checkstring(first(args)))+1;
  char *fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);

  if (SD.exists(fnbuf)) {
    SD.remove(fnbuf);
    free(fnbuf);
    return tee;
  }
  else {
    free(fnbuf);
    return nil;
  }
}

/*
  (sd-card-dir [mode])
  Print SD card directory and return nothing [mode 0, optional] or a uLisp string object [mode 1] or a List [mode 2].
*/
object *fn_SDCardDir (object *args, object *env) {
  (void) env;
  int mode = 0;
  if (args != NULL) {
    mode = checkinteger(first(args));
    if (mode > 2) mode = 2;
    if (mode < 0) mode = 0;
  }

  SD.begin(SDCARD_SS_PIN);
  File root = SD.open("/");
  int numTabs = 0;
  String dirstr;
  object* dirlist = NULL;

  if (mode == 0) {
    printDirectory(root, numTabs);
    return nil;
  }
  else if (mode == 1) {
    dirstr = printDirectoryStr(root, dirstr);
    return lispstring((char *)dirstr.c_str());
  }
  else if (mode == 2) {
    dirlist = printDirectoryList(root);
    return dirlist;
  }
  else return nil;
}

//Directory functions (modified Arduino PD code)
void printDirectory(File dir, int numTabs) {
  pfun_t pf = pserial;

  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      pfstring("  ", pf);
    }
    pfstring(entry.name(), pf);
    if (entry.isDirectory()) {
      pfstring("/\n", pf);
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      pfstring(" ", pf);
      pint(entry.size(), pf);
      pfstring("\n", pf);
    }
    entry.close();
  }
}

String printDirectoryStr(File dir, String dirstr) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    dirstr = dirstr + entry.name();
    if (entry.isDirectory()) {
      dirstr = dirstr + "/~%";
      dirstr = printDirectoryStr(entry, dirstr);
    } else {
      // files have sizes, directories do not
      dirstr = dirstr + " " + String(entry.size()) + "~%";
    }
    entry.close();
  }
  return dirstr;
}

object* printDirectoryList(File dir) {
  String dirstr = "";
  object* dirlist = NULL;

  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    dirstr = entry.name();
    if (entry.isDirectory()) {
      dirstr = dirstr + "/";
      dirlist = cons(printDirectoryList(entry), dirlist);
      dirlist = cons(lispstring((char *)dirstr.c_str()), dirlist);
    } else {
      dirlist = cons(lispstring((char *)dirstr.c_str()), dirlist);
    }
    
    entry.close();
  }
  return dirlist;
}
#endif


#if defined(rfm69)
/*
  Helper function:
  Prepare SDI for radio use. Not accessible via uLisp.
  (Not sure why this process is necessary - RFM69 driver probably changes SPI settings)
*/
void radioON () {
  SPI.begin();
  pinMode(PIN_RADIO_CS, OUTPUT);
  digitalWrite(PIN_RADIO_CS, LOW);
}

/*
  Helper function:
  Prepare SDI for other use. Not accessible via uLisp.
  (Not sure why this process is necessary - RFM69 driver probably changes SPI settings)
*/
void radioOFF () {
  pinMode(PIN_RADIO_CS, OUTPUT);
  digitalWrite(PIN_RADIO_CS, HIGH);
  SPI.begin();
  static SPISettings mySPISettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
}

/*
  (rfm69-begin)
  Start RFM69 radio module with pin and IRQ number assignments and reset it.
*/
object *fn_RFM69Begin (object *args, object *env) {
  (void) env;
 
  #if defined radiohead
    (void) args, (void) env;  //Radiohead library by default without sender/receiver and without network ID
  #else  
    int nodeid = checkinteger(first(args));
    int netid = checkinteger(second(args));
  #endif
  #if defined(rfm69) && defined(ARDUINO_PIMORONI_TINY2040)
    pinMode(RFM69_IRQ, INPUT_PULLUP); 
  #endif

  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  pfun_t pf = pserial;

  #if defined radiohead
    bool result = radio.init();
    if (!result)
    {
        pfstring("RH_RFM69 - Module init failed!", pf);
        return nil;
    }
    else
    {
        radio.setFrequency(FREQUENCY);
    }
  #else
    while(!radio.initialize(FREQUENCY, nodeid, netid))
    {
        pfstring("try to init...", pf);
        testescape();
        delay(100);
    }
  #endif

  if (IS_RFM69HCW) // Only for RFM69HCW & HW!
  {
      #if defined radiohead
        radio.setTxPower(13, true);
      #else
        radio.setHighPower(); 
      #endif
  }

  #if !defined radiohead
    radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)
    radio.encrypt(ENCRYPTKEY);
  #else
    radio.setEncryptionKey((uint8_t*)ENCRYPTKEY);
  #endif
  
  
  radioOFF();
  
  if (IS_RFM69HCW)
  {
      pfstring("RFM69 HCW initialized!\n", pf);
  }
  else
  {
      pfstring("RFM69 initialized!\n", pf);
  }
  pint(FREQUENCY, pf);
  #if !defined radiohead
    pserial(' ');
    pint(nodeid, pf);
    pserial(' ');
    pint(netid, pf);
  #endif
  pserial('\n');

  return nil;
}


/*
  (rfm69-send)
  Send string data package to specified receiver ID optionally requesting hardware acknowledge.
*/
object *fn_RFM69Send (object *args, object *env) {
  (void) env;
  
  if (args != NULL) {

    int receiver;
    bool ack = false;

    #if !defined radiohead
      receiver = checkinteger(first(args));
      args = cdr(args);
    #endif
    cstring(first(args), packet, PACKETLENGTH+1);   //build c-string from uLisp string, therefore includes additional '\0'

    #if !defined radiohead
      args = cdr(args);
      if (args != NULL) {
        ack = (first(args) == nil) ? false : true;
        }
    #endif
      
    radioON();
    #if defined radiohead
      bool result = radio.send((uint8_t*)packet, strlen(packet)+1);   //use *real* packet length for send, i.e. string may be shorter than PACKETLENGTH. '\0' sent too!
      if (!result)
      {
        radioOFF();
        pstring("RH_RFM69 send failed!", (pfun_t)pserial);
        return nil;   
      }
    #else
      radio.send(receiver, packet, strlen(packet)+1, ack);  //use *real* packet length for send, i.e. string may be shorter than PACKETLENGTH. '\0' sent too!
    #endif
    radioOFF();
    pstring(packet, (pfun_t)pserial);
    return tee;    
  }
  radioOFF();
  return nil;
}

/*
  (rfm69-receive)
  Retrieve string data package if something has been received.
*/
object *fn_RFM69Receive (object *args, object *env) {
  (void) env; (void) args;

  radioON();

  #if defined radiohead
    if (radio.available())
    {      
          bool result = radio.recv((uint8_t*)packet, &pctlen);   // RadioHead lib stores length of received packet in predefined global variable
          radioOFF();
          if (result)
          {
            packet[min(pctlen), PACKETLENGTH] = 0;  // add null terminating string for conversion into uLisp string
            return lispstring(packet);
          }
          else
          {
            pstring("RH_RFM69 receive failed!", (pfun_t)pserial);
            return nil;
          }
          
    }
  #else
    if (radio.receiveDone())
    {
          radioOFF();
          return lispstring((char*)radio.DATA);
    }
  #endif
  radioOFF();
  return nil;
}

/*
  (rfm69-get-rssi)
  Obtain signal strength reported at last transmit.
*/
object *fn_RFM69GetRSSI (object *args, object *env) {
  (void) env; (void) args;

  #if defined radiohead
    object* rssi = number(radio.rssiRead());
  #else
    object* rssi = number(radio.RSSI);
  #endif

  radioOFF();
  return rssi;
}
#endif


//
// OLED graphics and text routines - in part a modified copy of GFX routines in uLisp core
// No stream support to avoid major modification of uLisp core 
//

#if defined(oled_gfx)
/*
  (oled-begin [adr])
  Initialize OLED (optionally using I2C address adr, otherwise using default address #x3C (7 bit)/#x78 (8 bit)).
*/
object *fn_OledBegin (object *args, object *env) {
  (void) env;

  uint8_t adr = 0x78;
  if (args != NULL) {
    adr = (uint8_t)checkinteger(first(args));
  }

  oled.setI2CAddress(adr);
  if (!oled.begin()) {
    Serial.println("OLED Not Found!");
    return nil;
  }
  oled.clearBuffer();
  oled.setFont(u8g2_font_profont12_mr);
  return tee;
}  

/*
  (oled-clear)
  Clear OLED.
*/
object *fn_OledClear (object *args, object *env) {
  (void) args, (void) env;

  oled.clearBuffer();
  oled.sendBuffer();
  return nil;
}

/*
  (oled-set-rotation rot)
  Set rotation of screen. 0 = no rotation, 1 = 90 degrees, 2 = 180 degrees, 3 = 270 degrees, 4 = hor. mirrored, 5 = vert. mirrored.
*/
object *fn_OledSetRotation (object *args, object *env) {
  (void) env;
  uint8_t rot = (uint8_t)checkinteger(first(args));
  if (rot > 5) rot = 5;

  switch (rot) {
    case 0:
        oled.setDisplayRotation(U8G2_R0);
        break;
    case 1:
        oled.setDisplayRotation(U8G2_R1);
        break;
    case 2:
        oled.setDisplayRotation(U8G2_R2);
        break;
    case 3:
        oled.setDisplayRotation(U8G2_R3);
        break;
    case 4:
        oled.setDisplayRotation(U8G2_MIRROR);
        break;
    case 5:
        oled.setDisplayRotation(U8G2_MIRROR_VERTICAL);
        break; 
  }
  
  oled.sendBuffer();
  return nil;
}

/*
  (oled-set-color fg)
  Set foreground color for text and graphics. Colors are 0 (clear/black), 1 (set/white) and 2 (XOR).
*/
object *fn_OledSetColor (object *args, object *env) {
  (void) env;

  uint8_t col = (uint8_t)checkinteger(first(args));
  if (col > 2) col = 2;

  oled.setDrawColor(col);
  return nil;
}

/*
  (oled-write-char x y c)
  Write char c to screen at location x y.
*/
object *fn_OledWriteChar (object *args, object *env) {
  (void) env;

  oled.drawGlyph(checkinteger(first(args)), checkinteger(second(args)), checkchar(third(args)));
  oled.sendBuffer();
  return nil;
}

/*
  (oled-write-string x y str)
  Write string str to screen at location x y.
*/
object *fn_OledWriteString (object *args, object *env) {
  (void) env;

  int slength = stringlength(checkstring(third(args)))+1;
  char *oledbuf = (char*)malloc(slength);
  cstring(third(args), oledbuf, slength);
  oled.drawStr(checkinteger(first(args)), checkinteger(second(args)), oledbuf);
  oled.sendBuffer();
  free(oledbuf);
  return nil;
}

/*
  (oled-draw-pixel x y)
  Draw pixel at position x y (using color set before).
*/
object *fn_OledDrawPixel (object *args, object *env) {
  (void) env;
  oled.drawPixel(checkinteger(first(args)), checkinteger(second(args)));
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-line x0 y0 x1 y1)
  Draw a line between positions x0/y0 and x1/y1.
*/
object *fn_OledDrawLine (object *args, object *env) {
  (void) env;
  uint16_t params[4];
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawLine(params[0], params[1], params[2], params[3]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-hline x y w)
  Draw fast horizontal line at position X Y with length w.
*/
object *fn_OledDrawHLine (object *args, object *env) {
  (void) env;
  uint16_t params[3];
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawHLine(params[0], params[1], params[2]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-vline x y h)
  Draw fast vertical line at position X Y with length h.
*/
object *fn_OledDrawVLine (object *args, object *env) {
  (void) env;
  uint16_t params[3];
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawVLine(params[0], params[1], params[2]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-rect x y w h)
  Draw empty rectangle at x y with width w and height h.
*/
object *fn_OledDrawRect (object *args, object *env) {
  (void) env;
  uint16_t params[4];
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawFrame(params[0], params[1], params[2], params[3]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-fill-rect x y w h)
  Draw filled rectangle at x y with width w and height h.
*/
object *fn_OledFillRect (object *args, object *env) {
  (void) env;
  uint16_t params[4];
  for (int i=0; i<4; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawBox(params[0], params[1], params[2], params[3]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-circle x y r)
  Draw empty circle at position x y with radius r.
*/
object *fn_OledDrawCircle (object *args, object *env) {
  (void) env;
  uint16_t params[3];
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawCircle(params[0], params[1], params[2]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-fill-circle x y r)
  Draw filled circle at position x y with radius r.
*/
object *fn_OledFillCircle (object *args, object *env) {
  (void) env;
  uint16_t params[3];
  for (int i=0; i<3; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawDisc(params[0], params[1], params[2]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-draw-round-rect x y w h r)
  Draw empty rectangle at x y with width w and height h. Edges are rounded with radius r.
*/
object *fn_OledDrawRoundRect (object *args, object *env) {
  (void) env;
  uint16_t params[5];
  for (int i=0; i<5; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawRFrame(params[0], params[1], params[2], params[3], params[4]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-fill-round-rect)
  Draw filled rectangle at x y with width w and height h. Edges are rounded with radius r.
*/
object *fn_OledFillRoundRect (object *args, object *env) {
  (void) env;
  uint16_t params[5];
  for (int i=0; i<5; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawRBox(params[0], params[1], params[2], params[3], params[4]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-fill-triangle x0 y0 x1 y1 x2 y2)
  Draw filled triangle with corners at x0/y0, x1/y1 and x2/y2.
*/
object *fn_OledFillTriangle (object *args, object *env) {
  (void) env;
  uint16_t params[6];
  for (int i=0; i<6; i++) { params[i] = checkinteger(car(args)); args = cdr(args); }
  oled.drawTriangle(params[0], params[1], params[2], params[3], params[4], params[5]);
  oled.sendBuffer();
  return nil;
}

/*
  (oled-display-bmp fname x y)
  Open monochrome BMP file fname from SD if it exits and display it on screen at position x y (using the color set before).
*/
object *fn_OledDisplayBMP (object *args, object *env) {
  (void) env;

  SD.begin(SDCARD_SS_PIN);

  int slength = stringlength(checkstring(first(args)))+1;
  char *fnbuf = (char*)malloc(slength);
  cstring(first(args), fnbuf, slength);
  File file;

  if (!SD.exists(fnbuf)) {
    pfstring("File not found", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }
  int x = checkinteger(second(args));
  int y = checkinteger(third(args));
  (void) args;

  char buffer[BUFFERSIZE];
  file = SD.open(fnbuf);
  if (!file) { 
    pfstring("Problem reading from SD card", (pfun_t)pserial);
    free(fnbuf);
    file.close();
    return nil;
  }

  char b = file.read();
  char m = file.read();
  if ((m != 77) || (b != 66)) {
    pfstring("No BMP file", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  file.seek(10);
  uint32_t offset = SDRead32(file);
  SDRead32(file);
  int32_t width = SDRead32(file);
  int32_t height = SDRead32(file);
  int linebytes = floor(width / 8);
  int restbits = width % 8;
  if (restbits > 0) linebytes++;
  int zpad = 0;
  if ((linebytes % 4) > 0) {
    zpad = (4 - (linebytes % 4));
  }

  file.seek(28);
  uint16_t depth = file.read();
  if (depth > 1) { 
    pfstring("No monochrome bitmap file", (pfun_t)pserial);
    free(fnbuf);
    return nil;
  }

  file.seek(offset);

  int lx = 0;
  int bmpbyte = 0;
  int bmpbit = 0;

  for (int ly = (height - 1); ly >= 0; ly--) {
    for (int bx = 0; bx < linebytes; bx++) {
      bmpbyte = file.read();
      for (int bix = 0; bix < 8; bix++) {
        lx = (bx * 8) + bix;
        bmpbit = bmpbyte & (1 << (7-bix));
        if (bmpbit > 0) {
          oled.drawPixel(x+lx, y+ly);
        }
      }
    }
    //ignore trailing zero bytes
    if (zpad > 0) {
      for (int i = 0; i < zpad; i++) {
        file.read();
      }
    }
  }

  oled.sendBuffer();
  file.close();
  free(fnbuf);
  return nil;
}

/*
  (oled-show-bmp arr x y)
  Show bitmap image contained in uLisp array arr on screen at position x y (using color set before).
*/
object *fn_OledShowBMP (object *args, object *env) {

  object* array = first(args);
  if (!arrayp(array)) error2("argument is not an array");

  object *dimensions = cddr(array);
  if (listp(dimensions)) {
    if (listlength(dimensions) != 2) error2("array must be two-dimensional");
  }
  else error2("array must be two-dimensional");

  int x = checkinteger(second(args));
  int y = checkinteger(third(args));
  (void) args;

  int aw = abs(first(dimensions)->integer);
  int ah = abs(second(dimensions)->integer);
  int bit;
  object* subscripts;
  object* ox;
  object* oy;
  object* oyy;
  object** element;

  int bmpbit = 0;
  for (int ay = 0; ay < ah; ay++) {
    for (int ax = 0; ax < aw; ax++) {
      ox = number(ax);
      oy = number(ay);
      oyy = cons(oy, NULL);
      subscripts = cons(ox, oyy);
      element = getarray(array, subscripts, env, &bit);
      if (bit < 0) {
        error2("OLED draws monochrome image only");
      }
      else {
        bmpbit = abs(checkinteger(*element) & (1<<bit));
        if (bmpbit > 0) {
          oled.drawPixel(x+ax, y+ay);
        }
      }
      myfree(subscripts);
      myfree(oyy);
      myfree(oy);
      myfree(ox);
    }
  }
  oled.sendBuffer();

  return nil;
}
#endif


// Symbol names

//Added to standard GFX support, supported anytime
const char stringWriteText[] PROGMEM = "write-text";
const char stringWriteChar[] PROGMEM = "write-char";
#if defined bmp_ext
const char stringDisplayBMP[] PROGMEM = "display-bmp";
const char stringLoadBMP[] PROGMEM = "load-bmp";
const char stringLoadMono[] PROGMEM = "load-mono";
const char stringShowBMP[] PROGMEM = "show-bmp";
#endif

#if defined gif_ext
//GIF extension by David Johnson-Davies
const char stringDecodeGIF[] PROGMEM = "decode-gif";
const char stringEncodeGIF[] PROGMEM = "encode-gif";
const char stringPosterise[] PROGMEM = "posterise";
#endif

//extended keyboard function supported anytime
const char stringKeyboardGetKey[] PROGMEM = "keyboard-get-key";
const char stringKeyboardKeyPressed[] PROGMEM = "keyboard-key-pressed";

//Vector math supported anytime
const char stringRadToDeg[] PROGMEM = "rad-to-deg";
const char stringDegToRad[] PROGMEM = "deg-to-rad";
const char stringVectorSub[] PROGMEM = "vector-sub";
const char stringVectorAdd[] PROGMEM = "vector-add";
const char stringVectorNorm[] PROGMEM = "vector-norm";
const char stringScalarMult[] PROGMEM = "scalar-mult";
const char stringDotProduct[] PROGMEM = "dot-product";
const char stringCrossProduct[] PROGMEM = "cross-product";
const char stringVectorAngle[] PROGMEM = "vector-angle";

//String helper function from M5Cardputer editor version by hasn0life
const char stringSearchStr[] PROGMEM = "search-str";

//Extended SD card functions, supported anytime
const char stringSDFileExists[] PROGMEM = "sd-file-exists";
const char stringSDFileRemove[] PROGMEM = "sd-file-remove";
const char stringSDCardDir[] PROGMEM = "sd-card-dir";


#if defined(rfm69)
const char stringRFM69Begin[] PROGMEM = "rfm69-begin";
const char stringRFM69Send[] PROGMEM = "rfm69-send";
const char stringRFM69Receive[] PROGMEM = "rfm69-receive";
const char stringRFM69GetRSSI[] PROGMEM = "rfm69-get-rssi";
#endif


#if defined(oled_gfx)
const char stringOledBegin[] PROGMEM = "oled-begin";
const char stringOledClear[] PROGMEM = "oled-clear";
const char stringOledSetRotation[] PROGMEM = "oled-set-rotation";
const char stringOledSetColor[] PROGMEM = "oled-set-color";
const char stringOledWriteChar[] PROGMEM = "oled-write-char";
const char stringOledWriteString[] PROGMEM = "oled-write-string";
const char stringOledDrawPixel[] PROGMEM = "oled-draw-pixel";
const char stringOledDrawLine[] PROGMEM = "oled-draw-line";
const char stringOledDrawHLine[] PROGMEM = "oled-draw-hline";
const char stringOledDrawVLine[] PROGMEM = "oled-draw-vline";
const char stringOledDrawRect[] PROGMEM = "oled-draw-rect";
const char stringOledFillRect[] PROGMEM = "oled-fill-rect";
const char stringOledDrawCircle[] PROGMEM = "oled-draw-circle";
const char stringOledFillCircle[] PROGMEM = "oled-fill-circle";
const char stringOledDrawRoundRect[] PROGMEM = "oled-draw-round-rect";
const char stringOledFillRoundRect[] PROGMEM = "oled-fill-round-rect";
const char stringOledFillTriangle[] PROGMEM = "oled-fill-triangle";
const char stringOledDisplayBMP[] PROGMEM = "oled-display-bmp";
const char stringOledShowBMP[] PROGMEM = "oled-show-bmp";
#endif


// Documentation strings
//added to standard GFX support, supported anytime
const char docWriteText[] PROGMEM = "(write-text str)\n"
"Write string str to screen replacing DOS/cp437 character 'sharp s' (225) with UTF-8 sequence for Bodmer standard font.";
const char docWriteChar[] PROGMEM = "(write-char c)\n"
"Write char c to screen replacing DOS/cp437 character 'sharp s' (225) with UTF-8 sequence for Bodmer standard font.";
#if defined bmp_ext
const char docDisplayBMP[] PROGMEM = "(display-bmp fname x y)\n"
"Open BMP file fname from SD if it exits and display it on screen at position x y.";
const char docLoadBMP[] PROGMEM = "(load-bmp fname arr [offx] [offy])\n"
"Open BMP file fname from SD if it exits and copy it into the two-dimensional uLisp array provided.\n"
"Note that this allocates massive amounts of RAM. Use for small bitmaps/icons only.\n"
"When the image is larger than the array, only the upper leftmost area of the bitmap fitting into the array is loaded.\n"
"Providing offx and offy you may move the 'window' of the array to other parts of the bitmap (useful e.g. for tiling).";
const char docLoadMono[] PROGMEM = "(load-mono fname arr [offx] [offy])\n"
"Open monochrome BMP file fname from SD if it exits and copy it into the two-dimensional uLisp bit array provided.\n"
"Note that this allocates massive amounts of RAM. Use for small bitmaps/icons only.\n"
"When the image is larger than the array, only the upper leftmost area of the bitmap fitting into the array is loaded.\n"
"Providing offx and offy you may move the 'window' of the array to other parts of the bitmap (useful e.g. for tiling).";
const char docShowBMP[] PROGMEM = "(show-bmp arr x y [monocol])\n"
"Show bitmap image contained in uLisp array arr on screen at position x y.\n"
"The function automatically distinguishes between monochrome and color image arrays.\n"
"If monocol is provided, a monochrome image is painted with that color value.";
#endif

#if defined gif_ext
//GIF extension by David Johnson-Davies
const char docDecodeGIF[] PROGMEM = "(decode-gif stream [function])\n"
"Decodes a GIF file from the stream and plots it.\n"
"Alternatively a function fn(x y colours) can be provided that plots the colour index at x,y.";
const char docEncodeGIF[] = "(encode-gif stream xsize ysize colours [function] [noclear])\n"
"Encodes a GIF file by reading the display, and outputs the encoding with the specified number of colours to the stream.\n"
"Alternatively a function fn(x y colours) can be provided; it is called for each point, and should return the colour index.\n"
"Setting noclear to t can give a smaller file size for images with repetitive content.";
const char docPosterise[] = "(posterise col565 colours)\n"
"Converts a 16-bit colour col565 to a colour index in the \n"
"specified number of colours by posterising it, and returns it.";
#endif

//extended keyboard function supported anytime
const char docKeyboardGetKey[] PROGMEM = "(keyboard-get-key [translate] [state])\n"
"Waits for a key press and returns its ASCII value.\n"
"If translate is t the ASCII value is translated with function translate_key.\n"
"If state = 1, 2 or 3 (pressed, long-pressed, released) return key only if it was recognized with requested state .";
const char docKeyboardKeyPressed[] PROGMEM = "(keyboard-key-pressed [translate])\n"
"Returns key number and state as list if a key was pressed, nil if there is no keyboard data.\n"
"If translate is t the ASCII value is translated with function translate_key.";

//Vector math supported anytime
const char docRadToDeg[] PROGMEM = "(rad-to-deg n)\n"
"Convert radians to degrees.";
const char docDegToRad[] PROGMEM = "(deg-to-rad n)\n"
"Convert degree to radians.";
const char docVectorSub[] PROGMEM = "(vector-sub v1 v2)\n"
"Subtract vector v2 from vector v1 (lists with 3 elements).";
const char docVectorAdd[] PROGMEM = "(vector-add v1 v2)\n"
"Add vector v2 to vector v1 (lists with 3 elements).";
const char docVectorNorm[] PROGMEM = "(vector-norm v)\n"
"Calculate magnitude/norm of vector v (list with 3 elements).";
const char docScalarMult[] PROGMEM = "(scalar-mult v s)\n"
"Multiply vector v (list with 3 elements) by number s (scalar).";
const char docDotProduct[] PROGMEM = "(dot-product v1 v2)\n"
"Calculate dot product of two three-dimensional vectors v1, v2 (lists with 3 elements).";
const char docCrossProduct[] PROGMEM = "(cross-product v1 v2)\n"
"Calculate cross product of two three-dimensional vectors v1, v2 (lists with 3 elements).";
const char docVectorAngle[] PROGMEM = "(vector-angle v1 v2)\n"
"Calculate angle (rad) between two three-dimensional vectors v1, v2 (lists with 3 elements).";

//String helper function from M5Cardputer editor version by hasn0life
const char docSearchStr[] PROGMEM = "(search-str pattern target [startpos])\n"
"Returns the index of the first occurrence of pattern in target, or nil if it's not found\n"
"starting from startpos";

//Extended SD card functions, supported anytime
const char docSDFileExists[] PROGMEM = "(sd-file-exists filename)\n"
"Returns t if filename exists on SD card, otherwise nil.";
const char docSDFileRemove[] PROGMEM = "(sd-file-remove filename)\n"
"Delete file with filename. Returns t if successful, otherwise nil.";
const char docSDCardDir[] PROGMEM = "(sd-card-dir)\n"
"Print SD card directory and return nothing [mode 0, optional] or a uLisp string object [mode 1] or a List [mode 2].";


#if defined(rfm69)
const char docRFM69Begin[] PROGMEM = "(rfm69-begin nodeid netid)\n"
"Reset RFM69 module and initialize communication with frequency band, node id and net id.";
const char docRFM69Send[] PROGMEM = "(rfm69-send receiver pckgstr [ack])\n"
"Send string data package to specified receiver ID optionally requesting hardware acknowledge.";
const char docRFM69Receive[] PROGMEM = "(rfm69-receive)\n"
"Retrieve string data package if something has been received.";
const char docRFM69GetRSSI[] PROGMEM = "(rfm69-get-rssi)\n"
"Obtain signal strength reported at last transmit.";
#endif


#if defined(oled_gfx)
const char docOledBegin[] PROGMEM = "(oled-begin adr)\n"
"Initialize OLED (optionally using I2C address adr, otherwise using default address #x3C (7 bit)/#x78 (8 bit)).";
const char docOledClear[] PROGMEM = "(oled-clear)\n"
"Clear OLED.";
const char docOledSetRotation[] PROGMEM = "(oled-set-rotation rot)\n"
"Set rotation of screen. 0 = no rotation, 1 = 90 degrees, 2 = 180 degrees, 3 = 270 degrees, 4 = hor. mirrored, 5 = vert. mirrored.";
const char docOledSetColor[] PROGMEM = "(oled-set-color fg)\n"
"Set foreground color for text and graphics. Colors are 0 (clear/black), 1 (set/white) and 2 (XOR).";
const char docOledWriteChar[] PROGMEM = "(oled-write-char x y c)\n"
"Write char c to screen at location x y.";
const char docOledWriteString[] PROGMEM = "(oled-write-string x y str)\n"
"Write string str to screen at location x y.";
const char docOledDrawPixel[] PROGMEM = "(oled-draw-pixel x y)\n"
"Draw pixel at position x y (using color set before)";
const char docOledDrawLine[] PROGMEM = "(oled-draw-line x0 y0 x1 y1)\n"
"Draw a line between positions x0/y0 and x1/y1.";
const char docOledDrawHLine[] PROGMEM = "(oled-draw-hline x y w)\n"
"Draw fast horizontal line at position X Y with length w.";
const char docOledDrawVLine[] PROGMEM = "(oled-draw-vline x y h)\n"
"Draw fast vertical line at position X Y with length h.";
const char docOledDrawRect[] PROGMEM = "(oled-draw-rect x y w h)\n"
"Draw empty rectangle at x y with width w and height h.";
const char docOledFillRect[] PROGMEM = "(oled-fill-rect x y w h)\n"
"Draw empty rectangle at x y with width w and height h.";
const char docOledDrawCircle[] PROGMEM = "(oled-draw-circle x y r)\n"
"Draw empty circle at position x y with radius r.";
const char docOledFillCircle[] PROGMEM = "(oled-fill-circle x y r)\n"
"Draw filled circle at position x y with radius r.";
const char docOledDrawRoundRect[] PROGMEM = "(oled-draw-round-rect x y w h r)\n"
"Draw empty rectangle at x y with width w and height h. Edges are rounded with radius r.";
const char docOledFillRoundRect[] PROGMEM = "(oled-fill-round-rect x y w h r)\n"
"Draw filled rectangle at x y with width w and height h. Edges are rounded with radius r.";
const char docOledFillTriangle[] PROGMEM = "(oled-fill-triangle x0 y0 x1 y1 x2 y2)\n"
"Draw filled triangle with corners at x0/y0, x1/y1 and x2/y2.";
const char docOledDisplayBMP[] PROGMEM = "(oled-display-bmp fname x y)\n"
"Open monochrome BMP file fname from SD if it exits and display it on screen at position x y (using the color set before).";
const char docOledShowBMP[] PROGMEM = "(oled-show-bmp arr x y)\n"
"Show bitmap image contained in monochrome uLisp array arr on screen at position x y (using color set before).";
#endif


// Symbol lookup table
const tbl_entry_t lookup_table2[] PROGMEM = {

{ stringWriteText, fn_WriteText, 0211, docWriteText },
{ stringWriteChar, fn_WriteChar, 0211, docWriteChar },
#if defined bmp_ext
{ stringDisplayBMP, fn_DisplayBMP, 0233, docDisplayBMP },
{ stringLoadBMP, fn_LoadBMP, 0224, docLoadBMP },
{ stringLoadMono, fn_LoadMono, 0224, docLoadMono },
{ stringShowBMP, fn_ShowBMP, 0234, docShowBMP },
#endif

{ stringKeyboardGetKey, fn_KeyboardGetKey, 0202, docKeyboardGetKey },
{ stringKeyboardKeyPressed, fn_KeyboardKeyPressed, 0201, docKeyboardKeyPressed },

{ stringSearchStr, fn_searchstr, 0224, docSearchStr },

{ stringRadToDeg, fn_RadToDeg, 0211, docRadToDeg },
{ stringDegToRad, fn_DegToRad, 0211, docDegToRad },
{ stringVectorSub, fn_VectorSub, 0222, docVectorSub },
{ stringVectorAdd, fn_VectorAdd, 0222, docVectorAdd },
{ stringVectorNorm, fn_VectorNorm, 0211, docVectorNorm },
{ stringScalarMult, fn_ScalarMult, 0222, docScalarMult },
{ stringDotProduct, fn_DotProduct, 0222, docDotProduct },
{ stringCrossProduct, fn_CrossProduct, 0222, docCrossProduct },
{ stringVectorAngle, fn_VectorAngle, 0222, docVectorAngle },


 { stringSDFileExists, fn_SDFileExists, 0211, docSDFileExists },
 { stringSDFileRemove, fn_SDFileRemove, 0211, docSDFileRemove },
 { stringSDCardDir, fn_SDCardDir, 0201, docSDCardDir },


#if defined(rfm69)
  #if defined radiohead
    { stringRFM69Begin, fn_RFM69Begin, 0200, docRFM69Begin },
    { stringRFM69Send, fn_RFM69Send, 0211, docRFM69Send },
  #else
    { stringRFM69Begin, fn_RFM69Begin, 0222, docRFM69Begin },
    { stringRFM69Send, fn_RFM69Send, 0223, docRFM69Send },
  #endif
  { stringRFM69Receive, fn_RFM69Receive, 0200, docRFM69Receive },
  { stringRFM69GetRSSI, fn_RFM69GetRSSI, 0200, docRFM69GetRSSI },
#endif


#if defined(oled_gfx)
  { stringOledBegin, fn_OledBegin, 0201, docOledBegin },
  { stringOledClear, fn_OledClear, 0200, docOledClear },
  { stringOledSetRotation, fn_OledSetRotation, 0211, docOledSetRotation },
  { stringOledSetColor, fn_OledSetColor, 0211, docOledSetColor },
  { stringOledWriteChar, fn_OledWriteChar, 0233, docOledWriteChar },
  { stringOledWriteString, fn_OledWriteString, 0233, docOledWriteString },
  { stringOledDrawPixel, fn_OledDrawPixel, 0222, docOledDrawPixel },
  { stringOledDrawLine, fn_OledDrawLine, 0244, docOledDrawLine },
  { stringOledDrawHLine, fn_OledDrawHLine, 0233, docOledDrawHLine },
  { stringOledDrawVLine, fn_OledDrawVLine, 0233, docOledDrawVLine },
  { stringOledDrawRect, fn_OledDrawRect, 0244, docOledDrawRect },
  { stringOledFillRect, fn_OledFillRect, 0244, docOledFillRect },
  { stringOledDrawCircle, fn_OledDrawCircle, 0233, docOledDrawCircle },
  { stringOledFillCircle, fn_OledFillCircle, 0233, docOledFillCircle },
  { stringOledDrawRoundRect, fn_OledDrawRoundRect, 0255, docOledDrawRoundRect },
  { stringOledFillRoundRect, fn_OledFillRoundRect, 0255, docOledFillRoundRect },
  { stringOledFillTriangle, fn_OledFillTriangle, 0266, docOledFillTriangle },
  { stringOledDisplayBMP, fn_OledDisplayBMP, 0233, docOledDisplayBMP },
  { stringOledShowBMP, fn_OledShowBMP, 0233, docOledShowBMP },
#endif

#if defined gif_ext
  { stringDecodeGIF, fn_DecodeGIF, 0212, docDecodeGIF },
  { stringEncodeGIF, fn_EncodeGIF, 0246, docEncodeGIF },
  { stringPosterise, fn_Posterise, 0222, docPosterise },
#endif
};

// Table cross-reference functions - do not edit below this line

tbl_entry_t *tables[] = {lookup_table, lookup_table2};
const unsigned int tablesizes[] = { arraysize(lookup_table), arraysize(lookup_table2) };

const tbl_entry_t *table (int n) {
  return tables[n];
}

unsigned int tablesize (int n) {
  return tablesizes[n];
}
