















function testcase() {
        var count = 0;
        for (var i = 0; undefined;) {
            count++;
        }
        return count === 0;
    }
runTestCase(testcase);
