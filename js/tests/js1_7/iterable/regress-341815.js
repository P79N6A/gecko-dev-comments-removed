




































var gTestfile = 'regress-341815.js';

var BUGNUMBER = 341815;
var summary = 'Close hook crash';
var actual = 'No Crash';
var expect = 'No Crash';

var ialert = 0;




function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var globalToPokeGC = {};

  function make_iterator()
  {
    function generator() {
      try {
        yield 0;
      } finally {
        make_iterator();
      }
    }
    generator().next();
    globalToPokeGC = {};
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

  setTimeout('checkTest()', 10000);

  exitFunc ('test');
}

function init()
{
  
  setTimeout('test()', 5000);
}

var lastialert = 0;

function checkTest()
{
  
  
  
  

  if (ialert != lastialert)
  {
    lastialert = ialert;
    setTimeout('checkTest()', 10000);
    return;
  }

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

