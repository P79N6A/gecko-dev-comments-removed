














































var gTestfile = 'regress-192105.js';
var UBound = 0;
var BUGNUMBER = 192105;
var summary = 'Using |instanceof| to check if f() is called as constructor';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];










function f()
{
  actual = (this instanceof f);
}





status = inSection(1);
new f(); 
expect = true;
addThis();




status = inSection(2);
f(); 
expect = false;
addThis();





function F()
{
  new f();
}
status = inSection(3);
F(); 
expect = true;
addThis();




function G()
{
  f();
}
status = inSection(4);
G(); 
expect = false;
addThis();





var obj = {F:F, G:G};
status = inSection(5);
obj.F(); 
expect = true;
addThis();

status = inSection(6);
obj.G(); 
expect = false;
addThis();





function A()
{
  eval('F();');
}
status = inSection(7);
A(); 
expect = true;
addThis();


function B()
{
  eval('G();');
}
status = inSection(8);
B(); 
expect = false;
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
