
























#include <stdio.h>
#include <stdlib.h>

#include "xmalloc.h"

void *
xmalloc (size_t size)
{
    void *buf;

    buf = malloc (size);
    if (!buf) {
	fprintf (stderr, "Error: Out of memory. Exiting.\n");
	exit (1);
    }

    return buf;
}

void *
xcalloc (size_t nmemb, size_t size)
{
    void *buf;

    buf = calloc (nmemb, size);
    if (!buf) {
	fprintf (stderr, "Error: Out of memory. Exiting\n");
	exit (1);
    }

    return buf;
}

