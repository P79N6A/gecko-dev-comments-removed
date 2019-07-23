




































var gTestfile = 'regress-355105.js';

var BUGNUMBER = 355105;
var summary = 'decompilation of empty destructuring';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function () { try { } catch([] if true) { } catch(x) { } }
  expect = 'function () { try { } catch([] if true) { } catch(x) { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
