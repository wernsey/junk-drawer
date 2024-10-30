# Junk Drawer

Various utilities that I created over the years

## pocketmod

A simple command-line program to generate [Pocketmod][] PDFs.

It takes 8 PNG or JPG files as arguments and lays them out on a single page
in a PDF file for printing.

```
./modder -pg cover.png page1.png page2.png page3.png page4.png page5.png page6.png back.png
```

* `-o` Specifies the output file. The default is `pocketmod.pdf`
* `-f` lay the page out as a [pocketfold][] instead.
* `-m margin` specifies the margin around each image in its
  panel (default 10)
* `-g` add folding guides to the output PDF
* `-p` add page numbers to the output PDF. `-pp` adds
  page numbers to the front and back covers as well
* `-T title` adds a title to the PDF metadata
* `-A author` adds an author to the PDF metadata
* `-C creator` adds a creator to the PDF metadata
* `-S subject` adds a subject to the PDF metadata
* `-K keywords` adds keywords to the PDF metadata

PDFs are generated through [libharu][].

[Pocketmod]: https://pocketmod.com/
[libharu]: https://github.com/libharu/libharu
[pocketfold]: https://3skulls.itch.io/pocketfold

## screens

A simple Windows program that, when activated through its system tray icon,
allows you to take a quick screenshot of your desktop by pressing `Alt+S`

## CursEd

A [curses][]-based level editor for simple games.

The Windows version is built with MinGW and [pdcurses][]

[curses]: https://en.wikipedia.org/wiki/Curses_(programming_library)
[pdcurses]: https://pdcurses.org/

## mybas

A simple BASIC-to-C _compiler_.

## Other people's code

Some code written by other people that I have adapted for my own purposes:

### Picol

This is a modified version of [Picol][], Salvatore Sanfilippo ("antirez")'s Tcl interpreter
in ~500 lines.

This version turns it into a [STB][]-style single header library, so that it can be embedded
into another program as a command interpreter.

[Picol]: http://oldblog.antirez.com/post/picol.html
[STB]: https://github.com/nothings/stb/blob/master/docs/stb_howto.txt

### editor

Reference implementation for Sean Barret's [stb_tilemap_editor.h][]
to create a simple level editor for 2D games.

It uses ZSerge's [fenster][] framework for displaying the user interface.

[stb_tilemap_editor.h]: https://github.com/nothings/stb/blob/master/stb_tilemap_editor.h

### fenster-microui

An interface between ZSerge's [fenster][] framework and RXI's [microui][]
immediate mode GUI toolkit for quick cross-platform GUI programs.

[fenster]: https://github.com/zserge/fenster
[microui]: https://github.com/rxi/microui

### sbasic

A tiny BASIC interpreter from Chapter 7 of "C Power User's Guide" by Herbert Schildt, 1988.

The book can be found on [the Internet Archive](https://archive.org/details/cpowerusersguide00schi_0/mode/2up).

This version has been modified and modified and modified to add several features:

 - Converted it to C89 standard
 - Extracted the code as an API so you can script other programs with it
 - Call C-functions
 - Long variable names
 - Identifiers as labels, denoted as `@label`
 - `string$` variables...
   - ...and string literals in expressions
 - Logical operators `AND`, `OR` and `NOT`
 - `ON` statements
 - Comments with the `REM` keyword and `'` operator
 - Additional comparison operators `<>`, `<=` and `>=`
 - Escape sequences in string literals

 _You might notice a theme with all the interpreters in this collection._
