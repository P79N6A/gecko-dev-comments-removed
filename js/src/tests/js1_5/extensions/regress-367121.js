





































var BUGNUMBER = 367121;
var summary = 'self modifying script detection';
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
    actual = expect = 'Test skipped - Test must be run in the browser.';
    reportCompare(expect, actual, summary);
  }
  else if (typeof Script == 'undefined')
  {
    actual = expect = 'Test skipped - Test requires Script object..';
    reportCompare(expect, actual, summary);
  }
  else
  {
    gDelayTestDriverEnd = true;
  }

  exitFunc ('test');
}

function handleLoad()
{
  var iframe = document.body.appendChild(document.createElement('iframe'));
	var d = iframe.contentDocument;

	d.addEventListener("test", function(e) {
      s.compile("");
      Array(11).join(Array(11).join(Array(101).join("aaaaa")));
    }, true);

	var e = d.createEvent("Events");
	e.initEvent("test", true, true);
	var s = new Script("d.dispatchEvent(e);");
	s.exec();

  gDelayTestDriverEnd = false;
  reportCompare(expect, actual, summary);
  jsTestDriverEnd();
}

if (typeof window != 'undefined')
{
  window.onload = handleLoad;
}
