























































var gTestfile = 'regress-130451.js';
var UBound = 0;
var BUGNUMBER = 130451;
var summary = 'Array.prototype.sort() should not (re-)define .length';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var arr = [];
var cmp = new Function();





status = inSection(1);
arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
actual = arr.sort(cmp).length;
expect = 4;
addThis();

status = inSection(2);
arr = [0,1,2,3];
cmp = function(x,y) {return y-x;};
actual = arr.sort(cmp).length;
expect = 4;
addThis();

status = inSection(3);
arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
arr.length = 1;
actual = arr.sort(cmp).length;
expect = 1;
addThis();





arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
arr.sort(cmp);

status = inSection(4);
actual = arr.join();
expect = '0,1,2,3';
addThis();

status = inSection(5);
actual = arr.length;
expect = 4;
addThis();

status = inSection(6);
arr.length = 2;
actual = arr.join();
expect = '0,1';
addThis();

status = inSection(7);
arr.length = 4;
actual = arr.join();
expect = '0,1,,';  
addThis();






status = inSection(8);
var obj = new Object();
obj.sort = Array.prototype.sort;
obj.length = 4;
obj[0] = 0;
obj[1] = 1;
obj[2] = 2;
obj[3] = 3;
cmp = function(x,y) {return x-y;};
actual = obj.sort(cmp).length;
expect = 4;
addThis();







obj = new Object();
obj.sort = Array.prototype.sort;
obj.length = 4;
obj[0] = 3;
obj[1] = 2;
obj[2] = 1;
obj[3] = 0;
cmp = function(x,y) {return x-y;};
obj.sort(cmp);  
obj.join = Array.prototype.join;

status = inSection(9);
actual = obj.join();
expect = '0,1,2,3';
addThis();

status = inSection(10);
actual = obj.length;
expect = 4;
addThis();

status = inSection(11);
obj.length = 2;
actual = obj.join();
expect = '0,1';
addThis();







status = inSection(12);
obj.length = 4;
actual = obj.join();
expect = '0,1,2,3';
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
