





































var BUGNUMBER = 352613;
var summary = 'decompilation of |switch| |case| with computed value';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function () { switch(8) { case  7: a; case ('fafafa'.replace(/a/g, [1,2,3,4].map)): b; }}
  expect = 'function () {switch(8) { case  7: a; case "fafafa".replace(/a/g, [1,2,3,4].map): b; default:;}}';
  actual = f + '';
  compareSource(expect, actual, summary);

  try
  {
    f();
    throw new Error("no TypeError thrown calling map with undefined this");
  }
  catch(ex)
  {
    assertEq(ex instanceof TypeError, true,
             "No TypeError for Array.prototype.map with undefined this");
  }

  reportCompare(true, true, summary);
  exitFunc ('test');
}
