










 
 
function testcase(){
  Object.defineProperty(Object.prototype, "x", { get: function () { "use strict"; return this; } }); 
  if(!((5).x === 5)) return false;
  return true;
}

runTestCase(testcase);
