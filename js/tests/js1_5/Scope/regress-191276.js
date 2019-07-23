


















































var gTestfile = 'regress-191276.js';
var UBound = 0;
var BUGNUMBER = 191276;
var summary = 'Testing |this[name]| via Function.prototype.call(), apply()';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function F(name)
{
  return this[name];
}

function unused_function()
{
  F('a');
}

status = inSection(1);
actual = F.call({a: 'aaa'}, 'a');
expect = 'aaa';
addThis();

status = inSection(2);
actual = F.apply({a: 'aaa'}, ['a']);
expect = 'aaa';
addThis();




var obj = {a: 'aaa'};

status = inSection(3);
actual = F.call(obj, 'a');
expect = 'aaa';
addThis();

status = inSection(4);
actual = F.apply(obj, ['a']);
expect = 'aaa';
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
