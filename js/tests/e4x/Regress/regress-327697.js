





































gTestfile = 'regress-327697.js';

var summary = "Make XPConnect refuse to wrap e4x";
var BUGNUMBER = 327697;
var actual = 'No Hang';
var expect = 'No Hang';

printBugNumber(BUGNUMBER);
START(summary);
printStatus('This test runs in the browser only');

function init()
{
  try
  {
      var sel = document.getElementsByTagName("select")[0];
      sel.add(document.createElement('foo'), <bar/>);
  }
  catch(ex)
  {
      printStatus(ex + '');
  }
  TEST(1, expect, actual);
  END();
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();
}

if (typeof window != 'undefined')
{
    
    gDelayTestDriverEnd = true;

    document.write('<select></select>');
    window.addEventListener("load", init, false);
}
else
{
    TEST(1, expect, actual);
    END();
}

