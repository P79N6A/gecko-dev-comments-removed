
















































var gTestfile = 'regress-97921.js';
var UBound = 0;
var BUGNUMBER = 97921;
var summary = 'Testing with() statement with nested functions';
var cnYES = 'Inner value === outer value';
var cnNO = "Inner value !== outer value!";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var outerValue = '';
var innerValue = '';
var useWith = '';


function F(i)
{
  i = 0;
  if(useWith) with(1){i;}
  i++;

  outerValue = i; 
  F1 = function() {innerValue = i;}; 
  F1();
}


status = inSection(1);
useWith=false;
F(); 
actual = innerValue === outerValue;
expect = true;
addThis();

status = inSection(2);
useWith=true;
F(); 
actual = innerValue === outerValue;
expect = true;
addThis();


function G(i)
{
  i = 0;
  with (new Object()) {i=100};
  i++;

  outerValue = i; 
  G1 = function() {innerValue = i;}; 
  G1();
}


status = inSection(3);
G(); 
actual = innerValue === 101;
expect = true;
addThis();

status = inSection(4);
G(); 
actual = innerValue === outerValue;
expect = true;
addThis();




test();



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = areTheseEqual(actual);
  expectedvalues[UBound] = areTheseEqual(expect);
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


function areTheseEqual(yes)
{
  return yes? cnYES : cnNO
    }
