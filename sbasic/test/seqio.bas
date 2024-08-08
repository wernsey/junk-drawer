' The interpreter supports sequential text I/O
' through the OPEN, READ, WRITE and CLOSE functions

f = OPEN("data.txt", "w")
PRINT("Opened file for writing as #" + f)
WRITE(f, "Hello world!", 123, "some data")
WRITE(f, "A string", 456, "more data")
WRITE(f, "foo", 42, "A string value\nwith a newline")
CLOSE(f)

PRINT "Data written."

f = OPEN("data.txt", "r")
PRINT("Opened file for reading as #" + f)
FOR i = 1 TO 6
	PRINT("Field " + i + ": " + READ(f))
NEXT

READ f, &s$, &i, &n$
PRINT "Read: " + s$ + "; " + i + "; " + n$ 

CLOSE(f)
