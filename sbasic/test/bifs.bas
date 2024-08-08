REM Test for some built in functions
X$ = "This Is An Example String"
PRINT X$
PRINT "1234567890123456789012345"
PRINT "LEN: ", len(X$)
PRINT "INSTR('Example'): ", instr(X$, "Example")
PRINT "INSTR('foo'): ", instr(X$, "foo")
PRINT "MID(1,4): ", mid(x$, 1, 4)
PRINT "MID(12,7): ", mid(x$, 12, 7)
PRINT "MID(20,10): ", mid(x$, 20, 10)
PRINT "LEFT(4): ", left(x$, 4)
PRINT "RIGHT(6): ", right(x$, 6)
PRINT "UCASE: ", ucase(X$)
PRINT "LCASE: ", lcase(X$)
PRINT "IIF(1,2): ", iif(1,2)
PRINT "IIF(0,2): ", iif(0,2)
PRINT "IIF(1,2,3): ", iif(1,2,3)
PRINT "IIF(0,2,3): ", iif(0,2,3)
PRINT "10 + 2: ", 10 + 2
PRINT "\"10\" + 2: ", "10" + 2
PRINT "10 + 2 + X$: ", 10 + 2 + X$
PRINT "\"10\" + 2 + X$: ", "10" + 2 + X$
PRINT "10 - 2: ", 10 - 2
PRINT "\"10\" - 2: ", "10" - 2
PRINT "\"Hello \" + \"World\": ", "Hello " + "World"
PRINT "4^5: ", 4^5
PRINT "5 + -120: ", 5 + -120
PRINT "5 + +120: ", 5 + +120
PRINT "INT(\"10\" + 2) + 8: ", INT("10" + 2) + 8
PRINT "STR(10) + 2: ", STR(10) + 2

PRINT "MUX(2,one,two,three): ", MUX(2,"one","two","three")
PRINT "MUX(4,one,two,three): ", MUX(4,"one","two","three")
PRINT "DEMUX(two,one,two,three): ", DEMUX("two","one","two","three")
PRINT "DEMUX(four,one,two,three): ", DEMUX("four","one","two","three")
