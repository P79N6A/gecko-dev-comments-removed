












































var gTestfile = 'scope-002.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing visibility of outer function from inner function';
var cnCousin = 'Fred';
var cnColor = 'red';
var cnMake = 'Toyota';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



function Outer()
{

  function inner()
  {
    Outer.cousin = cnCousin;
    return Outer.cousin;
  }

  status = 'Section 1 of test';
  actual = inner();
  expect = cnCousin;
  addThis();
}


Outer();
status = 'Section 2 of test';
actual = Outer.cousin;
expect = cnCousin;
addThis();




function Car(make)
{
  this.make = make;
  Car.prototype.paint = paint;

  function paint()
  {
    Car.color = cnColor;
    Car.prototype.color = Car.color;
  }
}


var myCar = new Car(cnMake);
status = 'Section 3 of test';
actual = myCar.make;
expect = cnMake;
addThis();


myCar.paint();
status = 'Section 4 of test';
actual = myCar.color;
expect = cnColor;
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
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
