





























































var gTestfile = 'regress-178722.js';
var UBound = 0;
var BUGNUMBER = 178722;
var summary = 'arr.sort() should not output |undefined| when |arr| is empty';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var arr;



function f () {return arguments};
var arr5 = f();
arr5.__proto__ = Array.prototype;


status = inSection(5);
arr = arr5.sort();
actual = arr instanceof Array && arr.length === 0 && arr === arr5;
expect = true;
addThis();



function g() {return 1;}

status = inSection('5a');
arr = arr5.sort(g);
actual = arr instanceof Array && arr.length === 0 && arr === arr5;
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
