




































var gTestfile = 'regress-313630.js';

var BUGNUMBER = 313630;
var summary = 'Root access in js_fun_toString';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var f = Function("return 1");
Function("return 2");
expect = f.toSource(0);

var likeFunction = {
  valueOf: function() {
    var tmp = f;
    f = null;
    return tmp;
  },
  __proto__: Function.prototype
};

var likeNumber = {
  valueOf: function() {
    gc();
    return 0;
  }
};

var actual = likeFunction.toSource(likeNumber);
printStatus(expect === actual);

reportCompare(expect, actual, summary);
