







































#include "tests.h"
#include <iostream>

using namespace std;

JSAPITest *JSAPITest::list;

int main()
{
    int failures = 0;

    for (JSAPITest *test = JSAPITest::list; test; test = test->next) {
        string name = test->name();

        cout << name << endl;
        if (!test->init()) {
            cout << "TEST-UNEXPECTED-FAIL | " << name << " | Failed to initialize." << endl;
            failures++;
            continue;
        }

        if (test->run()) {
            cout << "TEST-PASS | " << name << " | ok" << endl;
        } else {
            cout << (test->knownFail ? "TEST-KNOWN-FAIL" : "TEST-UNEXPECTED-FAIL")
                 << " | " << name << " | " << test->messages() << endl;
            if (!test->knownFail)
                failures++;
        }
        test->uninit();
    }

    if (failures) {
        cout << "\n" << failures << " unexpected failure" << (failures == 1 ? "." : "s.") << endl;
        return 1;
    }
    cout << "\nPassed." << endl;
    return 0;
}
