










function testcase() {
        var prop = "66\u2029123";
        return prop === "66\u2029123" && prop[2] === "\u2029" && prop.length === 6;
    }
runTestCase(testcase);
