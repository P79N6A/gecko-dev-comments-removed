














































var gTestfile = 'regress-220367-002.js';
var UBound = 0;
var BUGNUMBER = 220367;
var summary = 'Regexp conformance test';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var re = /(a)|(b)/;

re.test('a');
status = inSection(1);
actual = RegExp.$1;
expect = 'a';
addThis();

status = inSection(2);
actual = RegExp.$2;
expect = '';
addThis();

re.test('b');
status = inSection(3);
actual = RegExp.$1;
expect = '';
addThis();

status = inSection(4);
actual = RegExp.$2;
expect = 'b';
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
