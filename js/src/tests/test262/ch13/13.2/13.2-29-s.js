












function testcase() {
        function foo() {"use strict";}
        return Object.getOwnPropertyDescriptor(foo, 
                                               "caller") === undefined;
}
runTestCase(testcase);
