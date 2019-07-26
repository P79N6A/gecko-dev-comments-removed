





function assertEq(setter) {
        if (setter > 10)
            return {assertEq: 3.3};
        return {__proto__: assertEq(setter + 1)};
    }
function testX() {
  var x = 2;
  var local0 = x;
  return { local0: local0 };
}
var resultsX = testX();
assertEq(resultsX.local0, 2);
gczeal(2);
assertEq(new (Proxy.createFunction({}, function(){}, function(){})), undefined);
