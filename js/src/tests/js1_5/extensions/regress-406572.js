





var BUGNUMBER = 406572;
var summary = 'JSOP_CLOSURE unconditionally replaces properties of the variable object - Browser only';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof window != 'undefined')
{
  var s = self;

  document.writeln(uneval(self));
  self = 1;
  document.writeln(uneval(self));

  if (1)
    function self() { return 1; }

  document.writeln(uneval(self));

  
  self = s;
}
else
{
  expect = actual = 'Test can only run in a Gecko 1.9 browser or later.';
  print(actual);
}
reportCompare(expect, actual, summary);


