
















































var gTestfile = 'regress-181834.js';
var UBound = 0;
var BUGNUMBER = 181834;
var summary = 'Testing scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];













function outer(N)
{
  var outer_d = 0;
  return inner(N);

  function inner(level)
  {
    outer_d++;

    if (level > 0)
      return inner(level - 1);
    else
      return outer_d;
  }
}





setDynamicScope(true);





var s = outer.toString();
eval(s);

status = inSection(1);
actual = outer(-5);
expect = 1;
addThis();

status = inSection(2);
actual = outer(0);
expect = 1;
addThis();

status = inSection(3);
actual = outer(5);
expect = 6;
addThis();





setDynamicScope(false);





eval(s);

status = inSection(4);
actual = outer(-5);
expect = 1;
addThis();

status = inSection(5);
actual = outer(0);
expect = 1;
addThis();

status = inSection(6);
actual = outer(5);
expect = 6;
addThis();




test();




function setDynamicScope(flag)
{
  if (this.Packages)
  {
    var cx = this.Packages.org.mozilla.javascript.Context.getCurrentContext();
    cx.setCompileFunctionsWithDynamicScope(flag);
  }
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
