




var BUGNUMBER = 380933;
var summary = 'Do not assert with uneval object with setter with modified proto';

printBugNumber(BUGNUMBER);
printStatus (summary);

var f = (function(){});
var y =
  Object.defineProperty({}, "p",
  {
    get: f,
    enumerable: true,
    configurable: true
  });
f.__proto__ = [];
uneval(y);

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");

