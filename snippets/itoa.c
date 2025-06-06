/*
 * `const char *itoa(int x, char *a, unsigned int n)`
 *
 * Converts the integer `x` to a string, writing the output
 * to the buffer `a` of size `n`
 *
 * Author: Werner Stoop
 * CC0 This work has been marked as dedicated to the public domain.
 * https://creativecommons.org/publicdomain/zero/1.0/
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const char *itoa(int x, char *a, unsigned int n) {
	char *b = a, *c, t;
	assert(a && n > 0);
	if(!x) {
		if(n < 2) goto error;
		a[0] = '0';
		a[1] = '\0';
		return a;
	}
	if(x < 0) {
		*b++ = '-';
		x = -x;
		if(--n == 0) goto error;
	}
	c = b;
	while(x) {
		div_t d = div(x, 10);
		*b++ = d.rem + '0'; /* *b++ = (x % 10) + '0'; */
		x = d.quot;         /* x /= 10; */
		if(--n == 0) goto error;
	}
	*b-- = '\0';
	while(c < b) {
		t = *c; *c++ = *b; *b-- = t;
	}
	return a;
error:
	*a = '\0';
	return a;
}

int main(int argc, char *argv[]) {
	int i;
	for(i = 1; i < argc; i++) {
		char buffer[32];
		int x = atoi(argv[i]);
		printf("%d => \"%s\"\n", x, itoa(x, buffer, sizeof buffer));
		//printf("%d => \"%s\"\n", x*3, itoa(x*3, buffer, sizeof buffer));
	}
	return 0;
}