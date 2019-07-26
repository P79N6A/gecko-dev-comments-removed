












function testcase() {
            function foo() {
                "use strict"; 
                for (var tempIndex in this) {
                    if (tempIndex==="arguments") {
                        return false;
                    }
                } 
                return true;
            }
            return foo();
}
runTestCase(testcase);