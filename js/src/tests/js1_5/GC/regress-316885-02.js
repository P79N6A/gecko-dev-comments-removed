




































var gTestfile = 'regress-316885-02.js';

var BUGNUMBER = 316885;
var summary = 'Unrooted access in jsinterp.c';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var str = "test";

var lval = {
  valueOf: function() {
    return str+"1";
  }
};

var ONE = 1;

var rval = {
  valueOf: function() {
    
    
    var tmp = "x"+lval;
    if (typeof gc == "function")
      gc();
    for (var i = 0; i != 40000; ++i) {
      tmp = 1e100 / ONE;
    }
    return str;
  }
};

expect = (str+"1" > str);
actual = (lval > rval);

reportCompare(expect, actual, summary);
