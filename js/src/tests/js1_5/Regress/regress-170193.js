













































var gTestfile = 'regress-170193.js';
var UBound = 0;
var BUGNUMBER = 170193;
var summary = 'adding property after middle-delete of function w duplicate formal args';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];




function f(a,a,b){}
f.c=42;
f.d=43;
delete f.c;  
f.e=44;

status = inSection(1);
actual = f.c;
expect = undefined;
addThis();

status = inSection(2);
actual = f.d;
expect = 43;
addThis();

status = inSection(3);
actual = f.e;
expect = 44;
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
