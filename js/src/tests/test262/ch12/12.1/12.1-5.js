










function testcase() {
        try {
            eval("if{};else if{}");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
