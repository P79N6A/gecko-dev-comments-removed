





var BUGNUMBER = 245795;
var summary = 'eval(uneval(function)) should be round-trippable';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof uneval != 'undefined')
{
  function a()
  {
    b = function() {};
  }

  var r = "function a() { b = function() {}; }";
  eval(uneval(a));

  var v = a.toString().replace(/[ \n]+/g, ' ');
  print(v)
 
  printStatus("[" + v + "]");

  expect = r;
  actual = v;

  reportCompare(expect, actual, summary);
}
