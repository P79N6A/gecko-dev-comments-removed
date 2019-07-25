





var BUGNUMBER = 351336;
var summary = 'decompilation of for initialization containing for';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = function () { for (("p" in a); 0;) { } }
  expect = 'function () {\n    for (("p" in a); false;) {\n    }\n}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
