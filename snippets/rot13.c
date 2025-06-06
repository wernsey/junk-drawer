/*
I swear I had a legitimate use for this.

Author: Werner Stoop
CC0 This work has been marked as dedicated to the public domain.
https://creativecommons.org/publicdomain/zero/1.0/
*/
#include <stdio.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
    FILE *i = stdin;
    FILE *o = stdout;

    /* Note that you might have \r\n related issues
    on windows if you use stdin and sdout directly.
    It caused me a bit of headscratching.
    */

    if(argc > 1) i = fopen(argv[1], "rb");
    if(!i) {
        perror(argv[1]);
    }
    if(argc > 2) o = fopen(argv[2], "wb");
    if(!o) {
        perror(argv[2]);
    }

    while(!feof(i)) {
        int c = fgetc(i);
        if(c == EOF) break;
        if(isalpha(c)) {
            if(isupper(c)) {
                c = ((c - 'A') + 13) % 26 + 'A';
            } else {
                c = ((c - 'a') + 13) % 26 + 'a';
            }
        }
        putc(c, o);
    }

    if(i != stdin) fclose(i);
    if(o != stdout) fclose(o);
    return 0;
}
