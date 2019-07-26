










function testcase() {
        try {
            eval("if{};else if{};else{}");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
