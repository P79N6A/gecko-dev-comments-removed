





var BUGNUMBER = 396969;
var summary = '11.7.3 - >>> should evaluate operands in order';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'o.valueOf, p.valueOf';
  var actualval;
  var expectval = 10;

  var o = {
    valueOf: (function (){ actual += 'o.valueOf'; return this.value}), 
    value:42
  };

  var p = {
    valueOf: (function (){ actual += ', p.valueOf'; return this.value}), 
    value:2
  };

  actualval = (o >>> p);

  reportCompare(expectval, actualval, summary + ': value');
  reportCompare(expect, actual, summary + ': order');

  exitFunc ('test');
}
