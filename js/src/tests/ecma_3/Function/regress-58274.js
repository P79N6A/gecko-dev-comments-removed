



































































var UBound = 0;
var BUGNUMBER = 58274;
var summary = 'Testing functions with double-byte names';
var ERR = 'UNEXPECTED ERROR! \n';
var ERR_MALFORMED_NAME = ERR + 'Could not find function name in: \n\n';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var sEval;
var sName;


sEval = "function f\u02B2() {return 42;}";
eval(sEval);
sName = getFunctionName(f\u02B2);


status = inSection(1);
actual = f\u02B2();
expect = 42;
addThis();


status = inSection(2);
actual = sName[0];
expect = sEval[9];
addThis();

status = inSection(3);
actual = sName[1];
expect = sEval[10];
addThis();



sEval = "function f\u02B2\u0AAA () {return 84;}";
eval(sEval);
sName = getFunctionName(f\u02B2\u0AAA);


status = inSection(4);
actual = f\u02B2\u0AAA();
expect = 84;
addThis();


status = inSection(5);
actual = sName[0];
expect = sEval[9];
addThis();

status = inSection(6);
actual = sName[1];
expect = sEval[10];
addThis();

status = inSection(7);
actual = sName[2];
expect = sEval[11];
addThis();





test();
























function getFunctionName(f)
{
  var s = condenseStr(f.toString());
  var re = /\s*function\s+(\S+)\s*\(/;
    var arr = s.match(re);

  if (!(arr && arr[1]))
    return ERR_MALFORMED_NAME + s;
  return arr[1];
}














function condenseStr(str)
{
  










  str = str.replace(/[\r\n]/g, '')
    return eval("'" + str + "'");
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
