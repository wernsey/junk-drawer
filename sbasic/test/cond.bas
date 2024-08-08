A = 1
B = 2
C = 0
PRINT "" + a + " " + b + " " + c

REM "these kinds of expressions didn't always work"
IF A then print "A"
IF A = 1 then print "A=1"
IF A = 2 then print "A=2"
IF B then print "B"
IF C then print "C"
IF A AND B then print "A AND B"
IF A AND C then print "A AND C"
IF A OR C then print "A OR C"

PRINT "~ fin ~"
