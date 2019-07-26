















function testcase() {
        var accessed = false;
        for (var i = 0; 2;) {
            accessed = true;
            break;
        }
        return accessed;
    }
runTestCase(testcase);
