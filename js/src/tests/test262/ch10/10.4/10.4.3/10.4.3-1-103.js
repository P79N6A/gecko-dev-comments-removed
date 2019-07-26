









function testcase(){
  Object.defineProperty(Object.prototype, "x", { get: function () { return this; } }); 
  if((5).x == 0) return false;
  if(!((5).x == 5)) return false;
  return true;
}

runTestCase(testcase);
