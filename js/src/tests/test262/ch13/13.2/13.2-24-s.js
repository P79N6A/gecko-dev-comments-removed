












function testcase() {
            function foo () {
                "use strict"; 
                for (var tempIndex in this) {
                    if (tempIndex==="caller") {
                        return false;
                    }
                } 
                return true;
            }
            return foo();
}
runTestCase(testcase);