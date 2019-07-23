















































#include "prinit.h"

#include <stdio.h>
#include <stdlib.h>







static char *compatible_version[] = {
    "4.0", "4.0.1", "4.1", "4.1.1", "4.1.2", "4.1.3",
    "4.2", "4.2.1", "4.2.2", "4.3", "4.4", "4.4.1",
    "4.5", "4.5.1",
    "4.6", "4.6.1", "4.6.2", "4.6.3", "4.6.4", "4.6.5",
    "4.6.6", "4.6.7", "4.6.8",
    "4.7", "4.7.1", "4.7.2", "4.7.3", "4.7.4", "4.7.5",
    "4.7.6",
    "4.8", "4.8.1", PR_VERSION
};








static char *incompatible_version[] = {
    "2.1 19980529",
    "3.0", "3.0.1",
    "3.1", "3.1.1", "3.1.2", "3.1.3",
    "3.5", "3.5.1",
    "4.8.9",
    "4.9", "4.9.1",
    "10.0", "11.1", "12.14.20"
};

int main(int argc, char **argv)
{
    int idx;
    int num_compatible = sizeof(compatible_version) / sizeof(char *);
    int num_incompatible = sizeof(incompatible_version) / sizeof(char *);

    printf("NSPR release %s:\n", PR_VERSION);
    for (idx = 0; idx < num_compatible; idx++) {
        if (PR_VersionCheck(compatible_version[idx]) == PR_FALSE) {
            fprintf(stderr, "Should be compatible with version %s\n",
                    compatible_version[idx]);
            exit(1);
        }
        printf("Compatible with version %s\n", compatible_version[idx]);
    }

    for (idx = 0; idx < num_incompatible; idx++) {
        if (PR_VersionCheck(incompatible_version[idx]) == PR_TRUE) {
            fprintf(stderr, "Should be incompatible with version %s\n",
                    incompatible_version[idx]);
            exit(1);
        }
        printf("Incompatible with version %s\n", incompatible_version[idx]);
    }

    printf("PASS\n");
    return 0;
}
