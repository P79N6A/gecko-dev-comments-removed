









    
function testcase() {
var x = 3;

return ("ab".replace("b", (function () { 
                                "use strict";
                                return function () {
                                    x = this;
                                    return "a";
                                }
                           })())==="aa") && (x===undefined);
}
runTestCase(testcase);