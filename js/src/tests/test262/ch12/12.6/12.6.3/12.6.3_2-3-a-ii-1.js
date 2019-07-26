















function testcase() {
        var accessed = false;
        var obj = { value: false };
        for (var i = 0; obj; ) {
            accessed = true;
            break;
        }
        return accessed;
    }
runTestCase(testcase);
