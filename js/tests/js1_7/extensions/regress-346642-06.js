






































var bug = 346642;
var summary = 'decompilation of destructuring assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  expect = 3;
  actual = '';
  "" + function() { [] = 3 }; actual = 3;
  actual = 3;
  reportCompare(expect, actual, summary + ': 1');

  var z = 6;
  var f = function (){for(let [] = []; false;) let z; return z}
  expect =  f();
  actual = eval(""+f)()
  reportCompare(expect, actual, summary + ': 2');

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
  "" + function () { [y([a]=b)] = z }; actual = 3;
  reportCompare(expect, actual, summary + ': 5');

  expect = 3;
  actual = '';
  "" + function () { for(;; ([[,]] = p)) { } }; actual = 3; 
  reportCompare(expect, actual, summary + ': 6');

  expect = 3;
  actual = '';
  actual = 1; try {for(x in (function ([y]) { })() ) { }}catch(ex){} actual = 3;
  reportCompare(expect, actual, summary + ': 7');

  exitFunc ('test');
}
