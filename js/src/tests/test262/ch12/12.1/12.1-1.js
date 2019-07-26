










function testcase() {
        try {
            eval("try{};catch(){}");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
