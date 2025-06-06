/*
** isort()  --  insertion sort
**
** Raymond Gardner   public domain   2/93
**
** qsort() compatible, but uses insertion sort algorithm.
*
* This version has been modified into a single header library.
*
* #define ISORT_IMPLEMENTATION
* #include "isort.h"
*
* The `isort_x()` function allows extra data passed to the comparator
* function.
*
*/
#ifndef ISORT_H
#define ISORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* for size_t definition */

void isort(void *base, size_t nmemb, size_t size,
            int (*comp)(const void *, const void *));

void isort_x(void *base, size_t nmemb, size_t size,
            int (*comp)(const void *, const void *, void *), void *data);

#ifdef ISORT_IMPLEMENTATION

void isort(void *base, size_t nmemb, size_t size,
            int (*comp)(const void *, const void *)) {

    char *i, *j, *lim;

    lim = (char *)base + nmemb * size;    /* pointer past end of array */
    for ( j = (char *)base, i = j+size; i < lim; j = i, i += size ) {
        for ( ; comp((void *)j, (void *)(j+size)) > 0; j -= size ) {
            char *a, *b;
            char tmp;
            size_t k = size;

            a = j;
            b = a + size;
            do {
                tmp = *a;
                *a++ = *b;
                *b++ = tmp;
            } while ( --k );
            if ( j == (char *)base )
                break;
        }
    }
}

void isort_x(void *base, size_t nmemb, size_t size,
            int (*comp)(const void *, const void *, void *), void *data) {

    char *i, *j, *lim;

    lim = (char *)base + nmemb * size;    /* pointer past end of array */
    for ( j = (char *)base, i = j+size; i < lim; j = i, i += size ) {
        for ( ; comp((void *)j, (void *)(j+size), data) > 0; j -= size ) {
            char *a, *b;
            char tmp;
            size_t k = size;

            a = j;
            b = a + size;
            do {
                tmp = *a;
                *a++ = *b;
                *b++ = tmp;
            } while ( --k );
            if ( j == (char *)base )
                break;
        }
    }
}

#endif /* ISORT_IMPLEMENTATION */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ISORT_H */