















function testcase() {
        var count = 0;
        for (var i = 0; NaN;) {
            count++;
        }
        return count === 0;
    }
runTestCase(testcase);
