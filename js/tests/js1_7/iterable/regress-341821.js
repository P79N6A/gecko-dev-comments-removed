




































var gTestfile = 'regress-341821.js';

var BUGNUMBER = 341821;
var summary = 'Close hook crash';
var actual = 'No Crash';
var expect = 'No Crash';

var ialert = 0;





function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function generator()
  {
    try {
      yield [];
    } finally {
      make_iterator();
    }
  }

  function make_iterator()
  {
    var iter = generator();
    iter.next();
    iter = null;
    if (typeof alert != 'undefined')
    {
      alert(++ialert);
    }
  }

  make_iterator();

  
  for (var i = 0; i != 50000; ++i) {
    var x = {};
  }

  print('done');
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

function init()
{
  
  setTimeout('runtest()', 5000);
}

function runtest()
{
  test();
  reportCompare(expect, actual, summary);
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
}

if (typeof window != 'undefined')
{
  
  gDelayTestDriverEnd = true;

  window.addEventListener("load", init, false);
}
else
{
  reportCompare(expect, actual, summary);
}

