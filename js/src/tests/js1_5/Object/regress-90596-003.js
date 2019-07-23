























































var gTestfile = 'regress-90596-003.js';
var UBound = 0;
var BUGNUMBER = 90596;
var summary = '[DontEnum] props (if overridden) should appear in for-in loops';
var cnCOMMA = ',';
var cnCOLON = ':';
var cnLBRACE = '{';
var cnRBRACE = '}';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var obj = {};


status = inSection(1);
obj = {toString:9};
actual = enumerateThis(obj);
expect = '{toString:9}';
addThis();

status = inSection(2);
obj = {hasOwnProperty:"Hi"};
actual = enumerateThis(obj);
expect = '{hasOwnProperty:"Hi"}';
addThis();

status = inSection(3);
obj = {toString:9, hasOwnProperty:"Hi"};
actual = enumerateThis(obj);
expect = '{toString:9, hasOwnProperty:"Hi"}';
addThis();

status = inSection(4);
obj = {prop1:1, toString:9, hasOwnProperty:"Hi"};
actual = enumerateThis(obj);
expect = '{prop1:1, toString:9, hasOwnProperty:"Hi"}';
addThis();



var s = '';

status = inSection(5);
s = 'obj = {toString:9}';
eval(s);
actual = enumerateThis(obj);
expect = '{toString:9}';
addThis();

status = inSection(6);
s = 'obj = {hasOwnProperty:"Hi"}';
eval(s);
actual = enumerateThis(obj);
expect = '{hasOwnProperty:"Hi"}';
addThis();

status = inSection(7);
s = 'obj = {toString:9, hasOwnProperty:"Hi"}';
eval(s);
actual = enumerateThis(obj);
expect = '{toString:9, hasOwnProperty:"Hi"}';
addThis();

status = inSection(8);
s = 'obj = {prop1:1, toString:9, hasOwnProperty:"Hi"}';
eval(s);
actual = enumerateThis(obj);
expect = '{prop1:1, toString:9, hasOwnProperty:"Hi"}';
addThis();



function A()
{
  status = inSection(9);
  var s = 'obj = {toString:9}';
  eval(s);
  actual = enumerateThis(obj);
  expect = '{toString:9}';
  addThis();
}
A();

function B()
{
  status = inSection(10);
  var s = 'obj = {hasOwnProperty:"Hi"}';
  eval(s);
  actual = enumerateThis(obj);
  expect = '{hasOwnProperty:"Hi"}';
  addThis();
}
B();

function C()
{
  status = inSection(11);
  var s = 'obj = {toString:9, hasOwnProperty:"Hi"}';
  eval(s);
  actual = enumerateThis(obj);
  expect = '{toString:9, hasOwnProperty:"Hi"}';
  addThis();
}
C();

function D()
{
  status = inSection(12);
  var s = 'obj = {prop1:1, toString:9, hasOwnProperty:"Hi"}';
  eval(s);
  actual = enumerateThis(obj);
  expect = '{prop1:1, toString:9, hasOwnProperty:"Hi"}';
  addThis();
}
D();




test();




function enumerateThis(obj)
{
  var arr = new Array();

  for (var prop in obj)
  {
    arr.push(prop + cnCOLON + obj[prop]);
  }

  var ret = addBraces(String(arr));
  return ret;
}


function addBraces(text)
{
  return cnLBRACE + text + cnRBRACE;
}





function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = sortThis(actual);
  expectedvalues[UBound] = sortThis(expect);
  UBound++;
}





function sortThis(sList)
{
  sList = compactThis(sList);
  sList = stripBraces(sList);
  var arr = sList.split(cnCOMMA);
  arr = arr.sort();
  var ret = String(arr);
  ret = addBraces(ret);
  return ret;
}





function compactThis(text)
{
  var charCode = 0;
  var ret = '';

  for (var i=0; i<text.length; i++)
  {
    charCode = text.charCodeAt(i);

    if (!isWhiteSpace(charCode) && !isQuote(charCode))
      ret += text.charAt(i);
  }

  return ret;
}


function isWhiteSpace(charCode)
{
  switch (charCode)
  {
  case (0x0009):
  case (0x000B):
  case (0x000C):
  case (0x0020):
  case (0x000A):  
  case (0x000D):  
    return true;
    break;

  default:
    return false;
  }
}


function isQuote(charCode)
{
  switch (charCode)
  {
  case (0x0027): 
  case (0x0022): 
    return true;
    break;

  default:
    return false;
  }
}





function stripBraces(text)
{
  
  var arr = text.match(/^\{(.*)\}$/);

  
  if (arr != null && arr[1] != null)
    return arr[1];
  return text;
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
