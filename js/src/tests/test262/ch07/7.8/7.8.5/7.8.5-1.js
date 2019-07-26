










function testcase() {
        try {
            eval("var regExp = /\\\rn/;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
