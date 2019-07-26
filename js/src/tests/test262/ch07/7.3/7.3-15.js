










function testcase() {
        var prop = "a\uFFFFa";
        return prop.length === 3 && prop !== "aa" && prop[1] === "\uFFFF";
    }
runTestCase(testcase);
