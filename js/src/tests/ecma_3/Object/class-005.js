


















































var gTestfile = 'class-005.js';
var i = 0;
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing the internal [[Class]] property of user-defined types';
var statprefix = 'Current user-defined type is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];


Calf.prototype= new Cow();





status = 'new Cow()';
actual = getJSClass(new Cow());
expect = 'Object';
addThis();

status = 'new Calf()';
actual = getJSClass(new Calf());
expect = 'Object';
addThis();



test();



function addThis()
{
  statusList[UBound] = status;
  actualvalue[UBound] = actual;
  expectedvalue[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalue[i], actualvalue[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statprefix + statusList[i];
}


function Cow(name)
{
  this.name=name;
}


function Calf(name)
{
  this.name=name;
}
