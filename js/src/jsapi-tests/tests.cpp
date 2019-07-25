







































#include "tests.h"
#include <stdio.h>

JSAPITest *JSAPITest::list;

int main(int argc, char *argv[])
{
    int failures = 0;
    const char *filter = (argc == 2) ? argv[1] : NULL;

    JS_SetCStringsAreUTF8();

    for (JSAPITest *test = JSAPITest::list; test; test = test->next) {
        const char *name = test->name();
        if (filter && strcmp(filter, name) != 0)
            continue;

        printf("%s\n", name);
        if (!test->init()) {
            printf("TEST-UNEXPECTED-FAIL | %s | Failed to initialize.\n", name);
            failures++;
            continue;
        }

        if (test->run()) {
            printf("TEST-PASS | %s | ok\n", name);
        } else {
            JSAPITestString messages = test->messages();
            printf("%s | %s | %.*s\n",
                   (test->knownFail ? "TEST-KNOWN-FAIL" : "TEST-UNEXPECTED-FAIL"),
                   name, (int) messages.length(), messages.begin());
            if (!test->knownFail)
                failures++;
        }
        test->uninit();
    }

    if (failures) {
        printf("\n%d unexpected failure%s.\n", failures, (failures == 1 ? "" : "s"));
        return 1;
    }
    printf("\nPassed.\n");
    return 0;
}
