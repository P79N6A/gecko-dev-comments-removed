





































var gTestfile = 'regress-249211.js';

var BUGNUMBER = 249211;
var summary = 'support export and import for 4xp';
var actual = '';
var expect = 'no error';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
try
{
  var o = {};
  var f = function(){};
  export *;
  import o.*;
  actual = 'no error';
}
catch(e)
{
  actual = 'error';
}

reportCompare(expect, actual, summary);

