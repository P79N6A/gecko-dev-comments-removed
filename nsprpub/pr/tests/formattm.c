






































#include "prtime.h"

#include <stdio.h>

int main()
{
    char buffer[256];
    PRTime now;
    PRExplodedTime tod;

    now = PR_Now();
    PR_ExplodeTime(now, PR_LocalTimeParameters, &tod);
    (void)PR_FormatTime(buffer, sizeof(buffer),
        "%a %b %d %H:%M:%S %Z %Y", &tod);
    printf("%s\n", buffer);
    (void)PR_FormatTimeUSEnglish(buffer, sizeof(buffer),
        "%a %b %d %H:%M:%S %Z %Y", &tod);
    printf("%s\n", buffer);
    return 0;
}
