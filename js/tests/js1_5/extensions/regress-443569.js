




































var gTestfile = 'regress-443569.js';

var BUGNUMBER = 443569;
var summary = 'Do not assert: OBJ_IS_NATIVE(obj)';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof window != 'undefined')
{
  
  gDelayTestDriverEnd = true;

  window.addEventListener("load", boom, false);
}
else
{
  reportCompare(expect, actual, summary);
}

function boom()
{
  var r = RegExp.prototype;
  r["-1"] = 0;
  Array.prototype.__proto__ = r;
  []["-1"];

  reportCompare(expect, actual, summary);

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
}


