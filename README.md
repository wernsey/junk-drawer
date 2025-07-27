# Junk Drawer

Various utilities that I created over the years, that may or may not
be useful and that I have nowhere else to put them.

## zine-maker

A simple command-line program to generate [Zine][]s in the form of [Pocketmod][] PDFs.

It takes 8 PNG or JPG files as arguments and lays them out on a single page
in a PDF file for printing.

See the [README.md](zine-maker/README.md) in that directory.

[Zine]: https://en.wikipedia.org/wiki/Zine
[Pocketmod]: https://pocketmod.com/

## screens

A simple Windows program that, when activated through its system tray icon,
allows you to take a quick screenshot of your desktop by pressing `Alt+S`

## CursEd

A [curses][]-based level editor for simple games.

The Windows version is built with MinGW and [pdcurses][]

[curses]: https://en.wikipedia.org/wiki/Curses_(programming_library)
[pdcurses]: https://pdcurses.org/

## new-project.shar

A shell script (technically a shell _archive_) to create a new C (or C++)
project. If you run it in an empty directory, it will create some directories
for your source, a basic `Makefile`, a `README.md`, a blank `LICENSE` and a
`.gitignore` file.

To get started, run these commands (assuming you want your project placed in
a directory called `project`):

```
mkdir project && cd project
sh ~/junk-drawer/new-project.shar
make
```

See the `README.md` inside the new directory for more details.

Note to self: the shar was created with the `-TV` options, as in
`shar -TV * > ~/junk-drawer/new-project.shar` if you want to recreate it

## mybas

A simple BASIC-to-C _compiler_.

## map

A dungeon generator for a Roguelike.

It is based on the "Rogue" algorithm, as described by Mark Damon Hughes
[Game Design: Article 07: Roguelike Dungeon Generation][dungeon], but with
some modifications to allow for locked doors (with the key placed sensibly)
and secret rooms.

The example program in [main.c](map/main.c) generates a SVG of the dungeon
level inspired by [watabou's One Page Dungeon][watabou]. It also uses the
same [Dyson Logos][dysonlogos]-style cross hatching based around poisson disc
sampling as described in [this devlog][hatching].

[dungeon]: https://web.archive.org/web/20131025132021/http://kuoi.org/~kamikaze/GameDesign/art07_rogue_dungeon.php
[watabou]: https://watabou.itch.io/one-page-dungeon
[hatching]: https://www.patreon.com/posts/hatching-in-1pdg-31716880
[dysonlogos]: https://dysonlogos.blog/2013/12/23/the-key-to-all-this-madness/

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

You might see theme here with all the level editors and map/maze generators.

[stb_tilemap_editor.h]: https://github.com/nothings/stb/blob/master/stb_tilemap_editor.h

### fenster-microui

An interface between ZSerge's [fenster][] framework and RXI's [microui][]
immediate mode GUI toolkit for quick cross-platform GUI programs.

[fenster]: https://github.com/zserge/fenster
[microui]: https://github.com/rxi/microui

### fenster-pocketmod

A demo for using `fenster_audio.h` from ZSerge's [fenster][] framework to play
module (MOD) music files through the [rombankzero/pocketmod][] library.

To play the music, drop the contents of the original
[pocketmod/songs](https://github.com/rombankzero/pocketmod/tree/master/songs)
directory into `fenster-pocketmod/songs`.

The **zine-maker** directory in this repo deals with a completely different
and unrelated concept that just so happens to also be called a "[Pocketmod][]".

[rombankzero/pocketmod]: https://github.com/rombankzero/pocketmod

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

### domsson

Converts OpenGameArt user [domsson][]'s CC0 bitmap fonts to byte arrays.

[domsson]: https://opengameart.org/users/domsson

## snippets

The snippets directory has some code that can be copied and pasted
into projects as needed.

Some of them from random places on the internet, some of them written
by myself. I take care to only include files under permissive licenses.

* `getopt.c` and `getopt.h` for command-line parameter processing originally
  posted on the `comp.sources.misc` newsgroup.
* `rot13.c` - A [rot13][] encoder/decoder
* `isort.h` - An insertion sort (by Raymond Gardner; I found it in the old
  SNIPPETS archive maintained by  Bob Stout)
* `pratt.c` - a math expression evaluator based on a Pratt parser as explained
  in the article [Pratt Parsers: Expression Parsing Made Easy][pratt] by Munificent
* `itoa.c` - converts an integer to a string

I added a CC0 waiver to the files written by me so it my intention for them to be used
without restriction is clear.

[rot13]: https://en.wikipedia.org/wiki/ROT13
[pratt]: https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/