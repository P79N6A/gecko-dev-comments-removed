










function testcase() {
        var prop = "66\u2028123";
        return prop === "66\u2028123" && prop[2] === "\u2028" && prop.length === 6;
    }
runTestCase(testcase);
