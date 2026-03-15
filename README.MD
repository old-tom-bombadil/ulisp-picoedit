# ulisp-picoedit --- Firmware with screen editor for uLisp on PicoCalc

This is a port of my screen editor for [LispBox/Lispdeck](https://github.com/ErsatzMoco/ulisp-lispbox).  This repository offers an extended version of ulisp-picocalc (see below). 

For easy **installation** with the current PicoCalc firmware on a Raspberry Pi **Pico 2 (!)** download "ulisp-picoedit2.ino.uf2" or "ulisp-picoedit2W.ino.uf2". To use it with a Pi Pico (old version) or other Pi Pico boards (experimental, not tested) clone repository and compile the software using the Arduino IDE (see http://www.ulisp.com/show?56ZO "Compiling and uploading uLisp").

Contains minor modifications to the current ulisp-picocalc version to enable software serial at PicoCalc's pin port, see below.
 
Additional functionality is provided by suitable uLisp C-extensions including RFM69 and vector math support, OLED graphics functions via I2C (default: disabled) and the new GIF library for uLisp (default: enabled) as well as functions for reading and displaying BMP files. Please see the extension file for further information. The Lisp library provides the editor and an extended ULOS system. The [ARM assembler](http://www.ulisp.com/show?2YRU) and the [compiler](http://www.ulisp.com/show?4W2I) by David Johnson-Davies are included in the Lisp library as well.

The editor features full SD card support, direct binding to uLisp symbols, full bracket matching and some keyboard shortcuts partially compatible with EMACS.


**The editor is written in uLisp. To invoke it type**

**`(se:sedit)`  or  `(se:sedit 'symbol)` where "symbol" can be any symbol name already present in uLisp.**


**Note:** Most shortcuts are identical to the LispBox/LispDeck editor version, but not all of them due to the different keyboard. The keyboard firmware of the PicoCalc also leads to a distinctive behaviour of the SHIFT/CTRL/ALT key combinations: To invoke a combination you do *not* need to *hold* the special key and press the second key - instead just press and release the special key, then press the second key of the shortcut. (To access the keys marked green on the keyboard it still *is* necessary to *hold* the SHIFT key since the green parts of the keyboard are handled by its internal firmware.)

* CTRL-q / CTRL-c --- quit editor and return to REPL
* CTRL-x / CTRL-b / CTRL-n --- discard current text buffer (i.e. new file)

* CTRL-k / CTRL-l / ALT-x --- delete line starting at cursor position (deleted part is copied before). If a selection exists, the selected line range will be copied and then be deleted.
* ALT-c --- copy current line or selected line range
* ALT-v --- insert copy buffer at cursor position

**Note:** ALT-v also serves as a rudimentary undo. If you accidentally delete part of a line or a selected line range using CTRL-k, CTRL-l or ALT-x you can insert that part again using ALT-v. 

* HOME --- move cursor to start of line
* CTRL-e / END --- move cursor to end of line
* CTRL-UP --- move cursor to beginning of buffer
* SHIFT-UP/SHIFT-DOWN --- move one page up or down

* CTRL-h / ALT-h --- **NEW** invoke help system. Press a key, then ENTER, then a list with all names within uLisp starting with the character you provided is shown read-only in the editor window. If there are no names starting with that character, "No entry" is shown. Otherwise, place the cursor on a list entry you want to read the help text for and press ENTER, then the built-in help text for that entry is shown read-only. If there is no help text available, "No doc" is shown. You may use ALT-c to copy a line (e.g. the function prototype). To return to your edit buffer press CTRL-h or ALT-h again (or F4, F6 or F7).

* CTRL-r / ALT-r --- **NEW** execute code of current line in editor immediately. If a selection exists, the selected line range will be enclosed in "(progn [...])" behind the scenes and then be executed immediately. See also "F5".
* CTRL-s / ALT-s --- **NEW** search downwards for provided string in buffer starting at cursor position. When found, the cursor jumps to the related text position.
* ALT-Space --- **NEW** send line or marked region via serial RX/TX and receive incoming answers. This utilizes the port at the left side of the PicoCalc via software serial (SerialPIO) and is tied to GP2 (PicoCalc TX) and GP3 (PicoCalc RX). The use of software serial prevents interference with the PicoCalc's USB chip that is tied in parallel to UART0 further down the port. The software serial has a baud speed limit, probably somewhere below 38400. At default the serial speed is set to 9600 baud, but you may adjust the global variable `se:baud` to try a higher value (use numbers like in `with-serial`). Any response incoming to the serial interface is fed into the read-only buffer of the editor *after* your code has been sent. To terminate receiving press any key slightly longer until the cursor reappears. To leave the read-only buffer and return to your code press ALT-Space again or F4, F6 or F7.

* CTRL-i / ALT-i --- **NEW**  set start line of selection for copy/paste/delete/execute
* CTRL-o / ALT-o --- **NEW**  set end line of selection for copy/paste/delete/execute
* CTRL-p / ALT-p --- **NEW**  unselect selected line range

* F1 --- toggle bracket matching on/off
* F2 --- check whether bracket under the cursor has a matching bracket in the buffer. If so, both are temporarily highlighted. (Use when continuous bracket matching is off.)
* F3 --- load text from SD card into buffer, discarding the present one
* F4 --- load the code of a symbol of your choice *read-only* into the editor. You may scroll through it and copy lines, but editing is blocked. To return to your text buffer press F4 again (or F6 or F7).
* F5 --- **NEW** either bind contents of the text buffer to a symbol of your choice and quit editor *or* run code (content of text buffer) immediately. To achieve the latter just press ENTER when the input line requests "Symbol name:" (i.e. don't provide a name). See also "CTRL-r/ALT-r".
* F6 --- show directory of SD card in *read-only* buffer. To return to your text buffer press F6 again (or F4 or F7).
* F7 --- load text from SD card *read-only* into the editor. You may scroll through it and copy lines, but editing is blocked. To return to your text buffer press F7 again (or F4 or F6).
* F8 --- save current buffer to backup file "lisp/BACKUP.CL" (overwriting old backup file). This is intended as a quick save function to prevent the worst case scenario of running out of battery before having saved your current work. It is recommended to use F8 at regular intervals. **Note:** The subdirectory "lisp" needs to exist beforehand, create it on the SD card before inserting it into PicoCalc.
* F9 --- delete a file on the SD card
* F10 --- save text buffer to SD card

Note: File names (excluding a suffix of 3 letters max.) may have a length of up to 17 characters *including* one or more directory names separated by a forward slash `/`. This is sufficient to access files with a filename length of 8 characters (suffix excluded) and a preceding directory name of 8 characters - or any combination of shorter file and directory names.

* ALT-F1 to ALT-F10 --- insert one of up to ten predefined snippets at cursor position

The snippets are defined in function `se:init` and reside in the global variable `se:sniplist` during runtime of the editor. You may change them according to your needs in file "LispboxLibrary.h" (re-flash your MCU with this firmware after change).

**Note:** This editor version has the abilty to enter special characters/umlauts. By default these are German umlauts:

* CTRL-a --- &auml;
* ALT-a --- &Auml;
* CTRL-u --- &uuml;
* ALT-u --- &Uuml;
* CTRL-0 --- &ouml; (Note: This is the zero key, had to change this because of conflict with range key)
* ALT-0 --- &Ouml; (Note: This is the zero key, had to change this because of conflict with range key)

Additionally:

* CTRL-z / ALT-z --- &szlig; ("sharp s") (Note: This is the z key, had to change this because of conflict with search key)

You may add your own combinations - please see sections marked with `#| SPECIAL CHARACTERS (UMLAUTS) |#` in function `se:sedit` within file `PicoEditLispLibrary.h`.

That way it is possible to use the screen editor for plain text editing as well.

**Important:** The standard font of the "Bodmer TFT-eSPI" library (which controls the display of the PicoCalc) uses an extended ASCII character set by Adafruit identical to the old DOS standard (cp437). So the set of the available special characters is limited, please see https://cdn-learn.adafruit.com/downloads/pdf/adafruit-gfx-graphics-library.pdf, page 16. To use a text file containing special characters (i.e. outside the standard ASCII set) created by PicoEdit on another machine, set the interpretation of the file to "DOS/cp437" for a correct display of all characters.

**Also important:** The "Bodmer TFT-eSPI" library uses UTF-8 encoding. Thus, *some* special characters in the font (those within a number range that was reserved for ASCII control characters historically) cannot be displayed via single ASCII numbers. Instead, they must be forwarded by their correct UTF-8 codes consisting of 2 or even 3 bytes. To display those characters (in the German set this concerns the "&szlig;" only) I had to include a really dirty hack: The uLisp graphics display command `(write-text)`, defined as an extension function `fn_WriteText` within `PicoEditExtensions.ino`, extracts those characters from the string to display and replaces them by their UTF-8 byte code. Please see the extension function on how this is done. The hack is necessary for *displaying* those characters on the PicoCalc *only*. Within the file the editor saves to SD card the characters are encoded with a single byte according to the mentioned "DOS/cp437" standard. The *file* is *not* encoded in UTF-8.


<br>
<br>
PicoEdit firmware is based on ulisp-arm by David Johnson-Davies:

# ulisp-arm
A version of the Lisp programming language for boards based on the ARM processor:

* Arduino Zero and MKRZero.
* Arduino Uno R4 Minima and WiFi.
* Adafruit ItsyBitsy M0, Feather M0, and Gemma M0.
* Adafruit Metro M4, ItsyBitsy M4, Feather M4, and Grand Central M4.
* Adafruit PyBadge and PyGamer.
* Adafruit CLUE and ItsyBitsy nRF52840.
* Raspberry Pi Pico and Raspberry Pi Pico W.
* BBC Micro Bit.
* Maxim MAX32620FTHR.
* Teensy 4.0/4.1.

For more information see: http://www.ulisp.com/
