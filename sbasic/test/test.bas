RANDOMIZE()

PRINT "It Works"  
PRINT "Foo$ is '", Foo$, "'"
GOSUB sub

PRINT "Random numbers: "; RND(20); RND(10,20); RND()

' This is possible because we don't check for EOL after the statements
a = 5 b = 6
print "a = ", a, "; b = ", b

foo$ = "The output value"

PRINT "String\twith\nxxxxx\rescapes and \"quotes\" and \\slashes\\ and hex escapes: '\x5A\x7A\x3F'"

INPUT "Enter a number [1-3], 4 for error> ", A
ON A GOSUB sub1, sub2, sub3, sub4

' \e can be used as escape for VT100-style terminals
PRINT "\e[31mThe end\e[0m"

' https://en.wikipedia.org/wiki/Box-drawing_character#Unix,_CP/M,_BBS
PRINT "\e(0\x6A\x6B\x6C\x6D\x6E\x71\x74\x75\x76\x77\x78\e(B"

END

@sub PRINT "In subroutine"
RETURN
@sub1 PRINT "In subroutine 1"
RETURN
@sub2 PRINT "In subroutine 2"
RETURN
@sub3 PRINT "In subroutine 3"
RETURN

@sub4 ERROR "Test error"
