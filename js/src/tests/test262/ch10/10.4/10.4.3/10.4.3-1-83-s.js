









    
function testcase() {
fnGlobalObject().f = function() {return this!==undefined;};
return (function () {return Function("\"use strict\";return f();")();})();
}
runTestCase(testcase);