











 
 function testcase(){
  Object.defineProperty(Object.prototype, "x", { get: function () { "use strict"; return this; } }); 
  if(!(typeof (5).x === "number")) return false;
  return true;
}

runTestCase(testcase);
