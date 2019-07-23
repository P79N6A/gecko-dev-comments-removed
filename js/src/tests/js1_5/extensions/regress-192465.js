































































var gTestfile = 'regress-192465.js';
var UBound = 0;
var BUGNUMBER = 192465;
var summary = 'Object.toSource() recursion should check stack overflow';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];






status = inSection(1);
var N = 90;
try
{
  make_deep_nest(N);
}
catch (e)
{
  
  
}
actual = 1;
expect = 1;
addThis();




test();














function make_deep_nest(level)
{
  var head = {};
  var cursor = head;

  for (var i=0; i!=N; ++i)
  {
    cursor.next = {};
    cursor = cursor.next;
  }

  cursor.toSource = function()
    {
      if (level != 0)
	return make_deep_nest(level - 1);
      return "END";
    }

  return head.toSource();
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
