



































































var UBound = 0;
var BUGNUMBER = 58274;
var summary = 'Testing identifiers with double-byte names';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


















var s0 =  'function Z';
var s1 =  '\u02b1(Z';
var s2 =  '\u02b2, b) {try { Z';
var s3 =  '\u02b3 : var Z';
var s4 =  '\u02b4 = Z';
var s5 =  '\u02b1; } catch (Z'
  var s6 =  '\u02b5) { for (var Z';
var s7 =  '\u02b6 in Z';
var s8 =  '\u02b5){for (1; 1<0; Z';
var s9 =  '\u02b7++) {new Array()[Z';
var s10 = '\u02b6] = 1;} };} }';





var sEval = s0 + s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
eval(sEval);







var arrID = getIdentifiers(Z\u02b1);





status = inSection(1);
actual = arrID[1];
expect = s1.charAt(0);
addThis();

status = inSection(2);
actual = arrID[2];
expect = s2.charAt(0);
addThis();

status = inSection(3);
actual = arrID[3];
expect = s3.charAt(0);
addThis();

status = inSection(4);
actual = arrID[4];
expect = s4.charAt(0);
addThis();

status = inSection(5);
actual = arrID[5];
expect = s5.charAt(0);
addThis();

status = inSection(6);
actual = arrID[6];
expect = s6.charAt(0);
addThis();

status = inSection(7);
actual = arrID[7];
expect = s7.charAt(0);
addThis();

status = inSection(8);
actual = arrID[8];
expect = s8.charAt(0);
addThis();

status = inSection(9);
actual = arrID[9];
expect = s9.charAt(0);
addThis();

status = inSection(10);
actual = arrID[10];
expect = s10.charAt(0);
addThis();





test();
























function getIdentifiers(f)
{
  var str = condenseStr(f.toString());
  var arr = str.split('Z');

  






  for (i in arr)
    arr[i] = arr[i].charAt(0);
  return arr;
}














function condenseStr(str)
{
  










  str = str.replace(/[\r\n]/g, '')
    return eval("'" + str + "'")
    }


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
