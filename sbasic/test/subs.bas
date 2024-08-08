' Tests for the `-u` command line parameter 
'
' ./basic -u baz -u err -u foo subs.bas

PRINT "Hello World"

x = 1

call &baz

PRINT "~ fin. ~"

END

@foo
	PRINT "in foo: ", x
	x = x + 1
	RETURN

' If you're calling a sub through `sb_gosub` you 
' can use either END or RETURN:

@bar
	PRINT "in bar: ", x	
	x = x * 2
	END

@baz
	PRINT "in baz: ", x
	GOSUB foo
	x = - x + 3
	RETURN

@err
	x = -100
	ERROR "This is a test"	
	END
