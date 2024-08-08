' The interpreter provides several functions that allows
' you to treat strings like "a,b,c" as a list of items
' `a`, `b` and `c`
' The functions aren't terribly efficient but opens up some
' possibilities that wouldn't otherwise be available.

L$ = LIST("foo", "bar", "baz")

PRINT "L$ = ", L$, "; len=", llen(l$)

L$ = LADD(L$, "fred")
L$ = LADD(L$, "quux")
 
PRINT "L$ = ", L$, "; len=", llen(l$) 

PRINT "LGET(): ", lget(l$, 3) 
PRINT "Lfind(baz): ", lfind(l$, "baz") 
PRINT "Lfind(fob): ", lfind(l$, "fob") 

PRINT "LLEN(''): ", llen("") 
PRINT "LLEN(','): ", llen(",") 
PRINT "LLEN('a,'): ", llen("a,") 
PRINT "LLEN(',,'): ", llen(",,") 
PRINT "LLEN(',a,'): ", llen(",a,") 
PRINT "Lget(',a,',1): ", lget(",a,",1) 

PRINT "LHEAD(L$): ", LHEAD(L$)
PRINT "LTAIL(L$): ", LTAIL(L$)
PRINT "LHEAD(''): ", LHEAD("")
PRINT "LTAIL(''): ", LTAIL("")
PRINT "LHEAD('a'): ", LHEAD("a")
PRINT "LTAIL('a'): ", LTAIL("a")
PRINT "LHEAD('a,'): ", LHEAD("a,")
PRINT "LTAIL('a,'): ", LTAIL("a,")
PRINT "LHEAD('a,b'): ", LHEAD("a,b")
PRINT "LTAIL('a,b'): ", LTAIL("a,b")
