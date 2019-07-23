





























































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



var arr1 = Array();
var arr2 = new Array();
var arr3 = [];
var arr4 = [1];
arr4.pop();


status = inSection(1);
arr = arr1.sort();
actual = arr instanceof Array && arr.length === 0 && arr === arr1;
expect = true;
addThis();

status = inSection(2);
arr = arr2.sort();
actual = arr instanceof Array && arr.length === 0 && arr === arr2;
expect = true;
addThis();

status = inSection(3);
arr = arr3.sort();
actual = arr instanceof Array && arr.length === 0 && arr === arr3;
expect = true;
addThis();

status = inSection(4);
arr = arr4.sort();
actual = arr instanceof Array && arr.length === 0 && arr === arr4;
expect = true;
addThis();


function g() {return 1;}

status = inSection('1a');
arr = arr1.sort(g);
actual = arr instanceof Array && arr.length === 0 && arr === arr1;
expect = true;
addThis();

status = inSection('2a');
arr = arr2.sort(g);
actual = arr instanceof Array && arr.length === 0 && arr === arr2;
expect = true;
addThis();

status = inSection('3a');
arr = arr3.sort(g);
actual = arr instanceof Array && arr.length === 0 && arr === arr3;
expect = true;
addThis();

status = inSection('4a');
arr = arr4.sort(g);
actual = arr instanceof Array && arr.length === 0 && arr === arr4;
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
