










function testcase() {
        var sum = 0;
        for (var i = 1; i <= 10; i++) {
            if (i === 6) {
                break
                ;
            }
            sum += i;
        }
        return sum === 15;
    }
runTestCase(testcase);
