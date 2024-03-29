












var UBound = 0;
var BUGNUMBER = 201987;
var summary = '|delete x.y| should return |true| if |x| has no property |y|';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
var x = {};
actual = delete x.y;
expect = true;
addThis();

status = inSection(2);
actual = delete {}.y;
expect = true;
addThis();

status = inSection(3);
actual = delete "".y;
expect = true;
addThis();

status = inSection(4);
actual = delete /abc/.y;
expect = true;
addThis();

status = inSection(5);
actual = delete (new Date()).y;
expect = true;
addThis();

status = inSection(6);
var x = 99;
actual = delete x.y;
expect = true;
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
