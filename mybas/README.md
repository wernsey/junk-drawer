BASIC-to-C compiler
===================

There are many BASIC-to-C compilers out there, but this one is mine.

I got the inspiration when I stumbled upon Peter Norvig's [BASIC interpreter][norvig]
in Python, which I'll refer to as [norvig][]. This one follows mostly the same syntax 
and symantics.

Normal statements and the math functions map quite simply to their C equivalents.

`GOTO` statements also map to the C `goto` statement, but the line numbers are
prefixed with `lbl_` to turn them into valid C labels.

`GOSUB` and `RETURN` uses a `jump:switch(addr)` block around the generated code as a 
so that the `addr` variable can be used for a computed goto. The `GOSUB` stores a value 
in the variable `ret`, and then the `RETURN` copies that value to `addr` and does a 
`goto jump;`.

`FOR` loops and their associated `NEXT` statements also make use of this computed goto
mechanism to repeat the loop.

Like [norvig][], there is no stack for `GOSUB` statements, so `GOSUB`
statements cannot be nested. I see no reason why the variable `ret` can't
be turned into a stack in the future.

`PRINT` statements do padding as per [norvig][]: `','` pads 15 spaces, `','` pads with 3 spaces. 
The padding is done such that there is always at least one space character.

An empty `PRINT` will just result in a newline being printed. It does not print a 
newline if you end a `PRINT` with a comma, eg. `PRINT S,`.

`DIM A(20)` declares an array indexable from `A(0)` to `A(20)`, thus the array holds 
*n + 1* elements. If an array is not `DIM`'ed , it is treated as a `DIM a(10)`.

Changes from the [norvig][] version:

* Added unary `+` and `-` operators.
* It supports multiple statements per line with the `':'` operator.
* It is case insensitive; Keywords and identifiers are converted to uppercase internally.
* `LET` keyword is optional; Lines starting with an identifier are treated as an implicit `LET`

TODO
----

* `AND`, `OR` and `NOT`
* Better string support.
* [norvig][] doesn't have an `INPUT` statement, but I see it as a necessity. It can be done once I have better string support.
* The compiler makes provision for using IDs instead of labels, but the parser
  doesn't. Likewise, line numbers can be made optional.
* FOR loops can be strange
  * In a `FOR` statement, if `STEP` is negative, then we should check if the loop variable is greater than the `TO` value in the comparison so that you can have statements like `FOR X = 10 TO 1 STEP -1`, see the [C64 FOR command](https://www.c64-wiki.com/wiki/FOR). 
  * We should probably also `assert()` that `STEP` is not zero in `FOR` loops.
  * The [C64 FOR-NEXT combination](https://www.c64-wiki.com/wiki/NEXT) actually uses a stack to track nested loops.
  * The identifier after the `NEXT` should be optional, eg `FOR I=1 TO 5 DO PRINT I : NEXT`
* [Wikipedia][] has more syntax that can be implemented, eg. `ON ... GOTO` and `DO ... LOOP WHILE`


[norvig]: https://github.com/norvig/pytudes/blob/master/ipynb/BASIC.ipynb
[Wikipedia]: https://en.wikipedia.org/wiki/BASIC#Syntax
[c64-wiki]: https://www.c64-wiki.com/wiki/C64-Commands
[dim]: https://www.c64-wiki.com/wiki/DIM
