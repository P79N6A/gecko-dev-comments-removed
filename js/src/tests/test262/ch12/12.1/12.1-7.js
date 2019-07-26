










function testcase() {
        try {
            eval("do{};while()");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
