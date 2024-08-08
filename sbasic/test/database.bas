' The interpreter has a simple key-value-store 
' database that allows you to insert a value with 
' the `POKE("key", "value")` function, and then 
' retrieve the value again with the `PEEK("key")`
' function.
' You specify the file to use as a database with the 
' `-d` command line option. 
' Run this file like so: $ ./basic -d demo.db test/database.bas 

RANDOMIZE

' `&x` is just syntactic sugar for `"x"`:
X = peek(&x)
PRINT "X is ", X
POKE "X", X+1

key$ = "A string value"
POKE &key$, key$ 
PRINT "key$ is ", peek(&key$)

Y = peek(&y)
PRINT "Y is ", Y
POKE "Y", RND()

PRINT "Z is ", peek(&z)

' You can insert a value for Z through this command:
' $ ./ppdb test.db poke Z 'A Value For Z'

PRINT "foo is ", peek(&foo)
POKE &foo, X + Y

