




































var gTestfile = 'regress-453024.js';

var BUGNUMBER = 453024;
var summary = 'Do not assert: vp + 2 + argc <= (jsval *) cx->stackPool.current->avail';
var actual = 'No Crash';
var expect = 'No Crash';

if (typeof window == 'undefined')
{
  reportCompare(true, true, summary + ': test requires browser.');
}
else
{
  gDelayTestDriverEnd = true;
  var j = 0;

  function test()
  {
    enterFunc ('test');
    printBugNumber(BUGNUMBER);
    printStatus (summary);
 
    for (var i = 0; i < 2000; ++i) {
      var ns = document.createElementNS("http://www.w3.org/1999/xhtml", "script");
      var nt = document.createTextNode("++j");
      ns.appendChild(nt);
      document.body.appendChild(ns);
    }

    gDelayTestDriverEnd = false;

    reportCompare(expect, actual, summary);

    jsTestDriverEnd();

    exitFunc ('test');
  }

  window.addEventListener('load', test, false);

}
