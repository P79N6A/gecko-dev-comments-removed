




































var gTestfile = 'regress-407720.js';

var BUGNUMBER = 407720;
var summary = 'js_FindClassObject causes crashes with getter/setter - Browser only';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);


var start = new Date();

if (typeof document != 'undefined')
{ 
  
  gDelayTestDriverEnd = true;
  document.write('<iframe onload="onLoad()"><\/iframe>');
}
else
{
  actual = 'No Crash';
  reportCompare(expect, actual, summary);
}

function onLoad() 
{

  if ( (new Date() - start) < 60*1000)
  {
    var x = frames[0].Window.prototype;
    x.a = x.b = x.c = 1;
    x.__defineGetter__("HTML document.all class", function() {});
    frames[0].document.all;

    
    frames[0].location = "about:blank";
  }
  else
  {
    actual = 'No Crash';

    reportCompare(expect, actual, summary);
    gDelayTestDriverEnd = false;
    jsTestDriverEnd();
  }
}
