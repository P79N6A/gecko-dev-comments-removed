





var BUGNUMBER = 346642;
var summary = 'decompilation of destructuring assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 3;
  actual = '';
  "" + function() { [] = 3 }; actual = 3;
  actual = 3;
  reportCompare(expect, actual, summary + ': 1');

  try
  {
    var z = 6;
    var f = eval('(function (){for(let [] = []; false;) let z; return z})');
    expect =  f();
    actual = eval("("+f+")")()
      reportCompare(expect, actual, summary + ': 2');
  }
  catch(ex)
  {
    
    var summarytrunk = 'let declaration must be direct child of block or top-level implicit block';
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk);
  }

  expect = 3;
  actual = '';
  "" + function () { for(;; [[a]] = [5]) { } }; actual = 3;
  reportCompare(expect, actual, summary + ': 3');

  expect = 3;
  actual = '';
  f = function () { return { set x([a]) { yield; } } }
  var obj = f();
  uneval(obj); actual = 3;
  reportCompare(expect, actual, summary + ': 4');

  expect = 3;
  actual = '';
  "" + function () { for(;; ([[,]] = p)) { } }; actual = 3;
  reportCompare(expect, actual, summary + ': 5');

  expect = 3;
  actual = '';
  actual = 1; try {for(x in (function ([y]) { })() ) { }}catch(ex){} actual = 3;
  reportCompare(expect, actual, summary + ': 6');

  exitFunc ('test');
}
