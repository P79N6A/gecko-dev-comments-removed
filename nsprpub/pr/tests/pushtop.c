






#include <stdio.h>
#include <stdlib.h>

#include "prio.h"

static PRIOMethods dummyMethods;

int main()
{
    PRDescIdentity topId, middleId, bottomId;
    PRFileDesc *top, *middle, *bottom;
    PRFileDesc *fd;

    topId = PR_GetUniqueIdentity("top");
    middleId = PR_GetUniqueIdentity("middle");
    bottomId = PR_GetUniqueIdentity("bottom");

    top = PR_CreateIOLayerStub(topId, &dummyMethods);
    middle = PR_CreateIOLayerStub(middleId, &dummyMethods);
    bottom = PR_CreateIOLayerStub(bottomId, &dummyMethods);

    fd = bottom;
    PR_PushIOLayer(fd, PR_TOP_IO_LAYER, middle);
    PR_PushIOLayer(fd, PR_TOP_IO_LAYER, top);

    top = fd;
    middle = top->lower;
    bottom = middle->lower;

    
    if (middle->higher != top) {
        fprintf(stderr, "middle->higher is wrong\n");
        fprintf(stderr, "FAILED\n");
        exit(1);
    }
    if (bottom->higher != middle) {
        fprintf(stderr, "bottom->higher is wrong\n");
        fprintf(stderr, "FAILED\n");
        exit(1);
    }

    top = PR_PopIOLayer(fd, topId);
    top->dtor(top);

    middle = fd;
    bottom = middle->lower;

    
    if (bottom->higher != middle) {
        fprintf(stderr, "bottom->higher is wrong\n");
        fprintf(stderr, "FAILED\n");
        exit(1);
    }

    middle = PR_PopIOLayer(fd, middleId);
    middle->dtor(middle);
    if (fd->identity != bottomId) {
        fprintf(stderr, "The bottom layer has the wrong identity\n");
        fprintf(stderr, "FAILED\n");
        exit(1);
    }
    fd->dtor(fd);

    printf("PASS\n");
    return 0;
}
