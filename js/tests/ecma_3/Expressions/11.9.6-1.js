













































var gTestfile = '11.9.6-1.js';
var UBound = 0;
var BUGNUMBER = 126722;
var summary = 'Testing the comparison |undefined === null|';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
if (undefined === null)
  actual = true;
else
  actual = false;
expect = false;
addThis();



status = inSection(2);
switch(true)
{
case (undefined === null) :
  actual = true;
  break;

default:
  actual = false;
}
expect = false;
addThis();



status = inSection(3);
function f3(x)
{
  var res = false;

  switch(true)
  {
  case (x === null) :
    res = true;
    break;

  default:
    
  }

  return res;
}

actual = f3(undefined);
expect = false;
addThis();



status = inSection(4);
function f4(arr)
{
  var elt = '';
  var res = false;

  for (i=0; i<arr.length; i++)
  {
    elt = arr[i];

    switch(true)
    {
    case (elt === null) :
      res = true;
      break;

    default:
      
    }
  }

  return res;
}

var arr = Array('a', undefined);
actual = f4(arr);
expect = false;
addThis();



status = inSection(5);
function f5(arr)
{
  var len = arr.length;

  for(var i=0; (arr[i]===undefined) && (i<len); i++)
    ; 
 
  return i;
}









var arrUndef = [ , undefined, eval('var x = 0'), this.NOT_A_PROPERTY, , ];
actual = f5(arrUndef);
expect = 5;
addThis();



status = inSection(6);
function f6(arr)
{
  var len = arr.length;

  for(var i=0; (arr[i]===null) && (i<len); i++)
    ; 

  return i;
}




actual = f6(arrUndef);
expect = 0;
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
