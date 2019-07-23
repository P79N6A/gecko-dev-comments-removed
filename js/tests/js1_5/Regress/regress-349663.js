




































var bug = 349663;
var summary = 'decompilation of Function with const *=';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f, g;

  f = function() { const h; for(null; h *= ""; null) ; }
  g = eval('(' + f + ')');

  expect = f + '';
  actual = g + '';

  print(f);
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
