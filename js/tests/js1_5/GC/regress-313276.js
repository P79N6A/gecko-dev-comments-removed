




































var gTestfile = 'regress-313276.js';

var BUGNUMBER = 313276;
var summary = 'Root strings';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var obj = {
  toString: function() {
    return "*TEST*".substr(1, 4);
  }
};

var TMP = 1e200;

var likeZero = {
  valueOf: function() {
    if (typeof gc == "function") gc();
    for (var i = 0; i != 40000; ++i) {
      var tmp = 2 / TMP;
      tmp = null;
    }
    return 0;
  }
}

  expect = "TEST";
actual = String.prototype.substr.call(obj, likeZero);

printStatus("Substring length: "+actual.length);
printStatus((expect === actual).toString());

reportCompare(expect, actual, summary);
