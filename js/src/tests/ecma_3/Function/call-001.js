






























































var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Applying Function.prototype.call to the Function object itself';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var self = this; 
var cnOBJECT_GLOBAL = self.toString();
var cnOBJECT_OBJECT = (new Object).toString();
var cnHello = 'Hello';
var cnRed = 'red';
var objTEST = {color:cnRed};
var f = new Function();
var g = new Function();


f = Function.call(self, 'return cnHello');
g = Function.call(objTEST, 'return cnHello');

status = 'Section A of test';
actual = f();
expect = cnHello;
captureThis();

status = 'Section B of test';
actual = g();
expect = cnHello;
captureThis();


f = Function.call(self, 'return this.toString()');
g = Function.call(objTEST, 'return this.toString()');

status = 'Section C of test';
actual = f();
expect = cnOBJECT_GLOBAL;
captureThis();

status = 'Section D of test';
actual = g();
expect = cnOBJECT_GLOBAL;
captureThis();


f = Function.call(self, 'return this.color');
g = Function.call(objTEST, 'return this.color');

status = 'Section E of test';
actual = f();
expect = undefined;
captureThis();

status = 'Section F of test';
actual = g();
expect = undefined;
captureThis();




test();



function captureThis()
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

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
