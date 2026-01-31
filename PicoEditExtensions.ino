/*
  PicoEdit uLisp Extension - Version 1.0
  Hartmut Grawe - github.com/ersatzmoco - Jan 2026

  Adopted search-str from
  M5Cardputer editor version by hasn0life - Nov 2024 - https://github.com/hasn0life/ulisp-sedit-m5cardputer

  Licensed under the MIT license: https://opensource.org/licenses/MIT
*/

// #define radiohead // Outcomment this to switch to LowPowerLab RFM69 library
// CURRENTLY MANDATORY BECAUSE OF UNSOLVED ISSUES WITHIN RADIOHEAD LIBRARY --- USE LOWPOWERLAB LIBRARY FOR NOW.

#if defined rfm69
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
  Write string str to screen replacing DOS/cp437 character "sharp s" (226) with UTF-8 sequence for Bodmer standard font.
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

  tft.startWrite();
  int starttime;
  for (int ly = (y + height - 1); ly >= y; ly--) {
    for (int lx = x; lx < (x + width); lx++) {
      tft.drawPixel(lx, ly, readBGR(file));
      //starttime = micros();
      //while (micros() < (starttime + 64));
    }
    //ignore trailing zero bytes
    if (zpad > 0) {
      for (int i = 0; i < zpad; i++) {
        file.read();
      }
    }
  }
  tft.endWrite();

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

        myfree(subscripts);
        myfree(oyy);
        myfree(oy);
        myfree(ox);
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
          *element = number((checkinteger(*element) & ~(1<<bit)) | bmpbit<<bit);

          myfree(subscripts);
          myfree(oyy);
          myfree(oy);
          myfree(ox);
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
      //starttime = micros();
      //while (micros() < (starttime + 64));

      myfree(subscripts);
      myfree(oyy);
      myfree(oy);
      myfree(ox);
    }
  }
  tft.endWrite();

  return nil;
}


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

  float a, b, sum;

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
            packet[pctlen] = 0;  // add null terminating string for conversion into uLisp string
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



// Symbol names

//Added to standard GFX support, supported anytime
const char stringWriteText[] PROGMEM = "write-text";
const char stringDisplayBMP[] PROGMEM = "display-bmp";
const char stringLoadBMP[] PROGMEM = "load-bmp";
const char stringLoadMono[] PROGMEM = "load-mono";
const char stringShowBMP[] PROGMEM = "show-bmp";

//extended keyboard function supported anytime
const char stringKeyboardGetKey[] PROGMEM = "keyboard-get-key";

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


// Documentation strings
//added to standard GFX support, supported anytime
const char docWriteText[] PROGMEM = "(write-text str)\n"
"Write string str to screen replacing DOS/cp437 character 'sharp s' (226) with UTF-8 sequence for Bodmer standard font.";
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

//extended keyboard function supported anytime
const char docKeyboardGetKey[] PROGMEM = "(keyboard-get-key [translate] [state])\n"
"Waits for a key press and returns its ASCII value.\n"
"If translate is t the ASCII value is translated with function translate_key.\n"
"If state = 1, 2 or 3 (pressed, long-pressed, released) return key only if it was recognized with requested state .";

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


// Symbol lookup table
const tbl_entry_t lookup_table2[] PROGMEM = {

{ stringWriteText, fn_WriteText, 0211, docWriteText },
{ stringDisplayBMP, fn_DisplayBMP, 0233, docDisplayBMP },
{ stringLoadBMP, fn_LoadBMP, 0224, docLoadBMP },
{ stringLoadMono, fn_LoadMono, 0224, docLoadMono },
{ stringShowBMP, fn_ShowBMP, 0234, docShowBMP },

{ stringKeyboardGetKey, fn_KeyboardGetKey, 0202, docKeyboardGetKey },

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
