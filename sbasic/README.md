A tiny BASIC interpreter
========================

From Chapter 7 of "C Power User's Guide" by Herbert Schildt, 1988.

The book can be found on [the Internet Archive](https://archive.org/details/cpowerusersguide00schi_0/mode/2up).

This version has been modified to add several features:

- Converted it to C89 standard
- Extracted the code as an API so you can script other programs with it
- Call C-functions
- Long variable names
- Identifiers as labels, denoted as `@label`
- `string$` variables...
  - ...and `"string literals"` in expressions
- Logical operators `AND`, `OR` and `NOT`
- String concatenation operator (`'+'`)
- `ON` statements
- Comments with the `REM` keyword and `'` character
- Comparison operators `<>`, `<=` and `>=`
- Escape sequences in string literals
- Functions base on those from [C64 BASIC](https://www.c64-wiki.com/wiki/BASIC)
  - [RND](https://www.c64-wiki.com/wiki/RND)
  - [LEN](https://www.c64-wiki.com/wiki/LEN)
  - [LEFT$](https://www.c64-wiki.com/wiki/LEFT$), [RIGHT$](https://www.c64-wiki.com/wiki/RIGHT$) 
     and [MID$](https://www.c64-wiki.com/wiki/MID$)
  - [INSTR](https://www.c64-wiki.com/wiki/INSTR)
- `upper(s$)` and `lower(s$)` functions.
- `IIF(cond, trueVal, falseVal)` built-in function.
- References through the `&` operator
  - `&var` is just syntactic sugar for `"var"`

TODO
----

[ ] `get_token()` should make sure tokens don't exceed `TOKEN_SIZE`

Notes
-----

This statement does not work:

```
PRINT "Some text" REM a comment
```

I'm not sure if I should consider it a bug.

Links
-----

These articles might be interesting:

* [BASIC Variables & Strings — with Commodore](https://www.masswerk.at/nowgobang/2020/commodore-basic-variables)
* [Commodore 64 BASIC 2.0 garbage collection, strings assignments in loops](https://retro64.altervista.org/blog/commodore-64-basic-2-0-garbage-collection-strings-assignments-loop/)
