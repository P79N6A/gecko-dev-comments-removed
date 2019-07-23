



























































var gTestfile = 'regress-123002.js';
var LENGTH_RHINO = 1;
var LENGTH_SPIDERMONKEY = 3;
var UBound = 0;
var BUGNUMBER = 123002;
var summary = 'Testing Error.length';
var QUOTE = '"';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





var LENGTH_EXPECTED = inRhino()? LENGTH_RHINO : LENGTH_SPIDERMONKEY;




var errObjects = [new Error(), new EvalError(), new RangeError(),
		  new ReferenceError(), new SyntaxError(), new TypeError(), new URIError()];


for (var i in errObjects)
{
  var err = errObjects[i];
  status = inSection(quoteThis(err.name));
  actual = Error.length;
  expect = LENGTH_EXPECTED;
  addThis();
}




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


function quoteThis(text)
{
  return QUOTE + text + QUOTE;
}
