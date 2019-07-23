

















































var gTestfile = 'regress-137000.js';
var UBound = 0;
var BUGNUMBER = 137000;
var summary = 'Function param or local var with same name as a function prop';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];






function f(x)
{
}

status = inSection(1);
f.x = 12;
actual = f.x;
expect = 12;
addThis();










function parentObject(p)
{
  this.p = 1;
}

function childObject()
{
  parentObject.call(this);
}
childObject.prototype = parentObject;

status = inSection(2);
var objParent = new parentObject();
actual = objParent.p;
expect = 1;
addThis();

status = inSection(3);
var objChild = new childObject();
actual = objChild.p;
expect = 1;
addThis();







function Base(id)
{
}

function Child(id)
{
  this.prop = id;
}
Child.prototype=Base;

status = inSection(4);
var c1 = new Child('child1');
actual = c1.prop;
expect = 'child1';
addThis();






function BaseX(id)
{
}

function ChildX(id)
{
  this.id = id;
}
ChildX.prototype=BaseX;

status = inSection(5);
c1 = new ChildX('child1');
actual = c1.id;
expect = 'child1';
addThis();









function g()
{
  var propA = g.propA;
  var propB = g.propC;

  this.getVarA = function() {return propA;}
  this.getVarB = function() {return propB;}
}
g.propA = 'A';
g.propB = 'B';
g.propC = 'C';
var obj = new g();

status = inSection(6);
actual = obj.getVarA(); 
expect = 'A';
addThis();

status = inSection(7);
actual = obj.getVarB(); 
expect = 'C';
addThis();












function setFProperty(val)
{
  F.propA = val;
}

function F()
{
  var propA = 'Local variable in F';
}

status = inSection(8);
setFProperty('Hello');
actual = F.propA; 
expect = 'Hello';
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
