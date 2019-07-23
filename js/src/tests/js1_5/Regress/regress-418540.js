




































var gTestfile = 'regress-418540.js';

var BUGNUMBER = 418540;
var summary = 'Do not assert: OBJ_IS_NATIVE(obj)';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof window == 'undefined')
  {
    expect = actual = 'Browser test only - skipped';
    reportCompare(expect, actual, summary);
  }
  else
  {
    gDelayTestDriverEnd = true;
    window.onload = boom;
  }

  exitFunc ('test');
}

function boom()
{
  var p;
  var b = document.createElement("body");
  var v = document.createElement("div");
  b.getAttribute("id")
  v.getAttribute("id")
  for (p in v) { }
  for (p in b) { }
  b.__proto__ = [];
  try { aC(v, null); } catch(e) { }
  try { aC(b, null); } catch(e) { }

  setTimeout(check, 1000);
}

function aC(r, n) { r.appendChild(n); }

function check()
{
  expect = actual = 'No Crash';
  gDelayTestDriverEnd = false;
  reportCompare(expect, actual, summary);
  jsTestDriverEnd();
}
