




































var gTestfile = 'regress-341877-02.js';

var BUGNUMBER = 341877;
var summary = 'GC hazard with for-in loop';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var obj = { };

var prop = "xsomePropety".substr(1);

obj.first = "first"

  obj[prop] = 1;

for (var elem in obj) {
  var tmp = elem.toString();
  delete obj[prop];
  
  prop = "xsomePropety".substr(2);
  obj[prop] = 2;
  delete obj[prop];
  prop = null;
  if (typeof gc == 'function')
    gc();
  for (var i = 0; i != 50000; ++i) {
    var tmp = 1 / 3;
    tmp /= 10;
  }
  for (var i = 0; i != 1000; ++i) {
    
    
    
    var tmp2 = Array(12).join(' ');
  }
}
 
reportCompare(expect, actual, summary);
