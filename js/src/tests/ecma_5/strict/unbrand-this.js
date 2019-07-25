








function strict() {
  "use strict";
  this.insert = function(){ bar(); };
  function bar() {}
}

var exception;


exception = null;
try {
  strict.call(undefined);
} catch (x) {
  exception = x;
}
assertEq(exception instanceof TypeError, true);


exception = null;
try {
  strict.call(null);
} catch (x) {
  exception = x;
}
assertEq(exception instanceof TypeError, true);


exception = null;
try {
  strict.call({});
} catch (x) {
  exception = x;
}
assertEq(exception, null);

reportCompare(true, true);
