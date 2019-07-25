





var BUGNUMBER = 398485;
var summary = 'Date.prototype.toLocaleString should not clamp year';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var d;
  var y;
  var l;
  var maxms = 8640000000000000;

  d = new Date(-maxms );
  y = d.getFullYear();
  l = d.toLocaleString();
  print(l);

  actual = y;
  expect = -271821;
  reportCompare(expect, actual, summary + ': check year');

  actual = l.match(new RegExp(y)) + '';
  expect = y + '';
  reportCompare(expect, actual, summary + ': check toLocaleString');

  d = new Date(maxms );
  y = d.getFullYear();
  l = d.toLocaleString();
  print(l);

  actual = y;
  expect = 275760;
  reportCompare(expect, actual, summary + ': check year');

  actual = l.match(new RegExp(y)) + '';
  expect = y + '';
  reportCompare(expect, actual, summary + ': check toLocaleString');

  exitFunc ('test');
}
