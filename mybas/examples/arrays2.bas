10  DIM B(20,30)
20  FOR I = 1 TO 20 
30  FOR J = 1 TO 30
40   LET B(I,J) = I * 100 + J
50  NEXT J
60  NEXT I
70  FOR I = 1 TO 20
80  FOR J = 1 TO 30
90   PRINT "B(" I "," J ") is " B(I,J) "(" (I * 100 + J) ")"
100 NEXT J
110 NEXT I

