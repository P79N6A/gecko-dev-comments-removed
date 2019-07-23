














































var gTestfile = 'regress-146596.js';
var UBound = 0;
var BUGNUMBER = 146596;
var summary = "Shouldn't crash when catch parameter is 'hidden' by varX";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];












function F()
{
  try
  {
    return "A simple exception";
  }
  catch(e)
  {
    var e = "Another exception";
  }

  return 'XYZ';
}

status = inSection(1);
actual = F();
expect = "A simple exception";
addThis();













function f(obj)
{
  var res = [];

  try
  {
    throw 42;
  }
  catch(e)
  {
    with(obj)
    {
      var e;
      res[0] = e; 
    }

    res[1] = e;   
  }

  res[2] = e;     
  return res;
}

status = inSection(2);
actual = f({e:24});
expect = [24, 42, undefined];
addThis();





test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual.toString();
  expectedvalues[UBound] = expect.toString();
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
