










function testcase() {
        try {
            eval("function x,y,z(){}");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
