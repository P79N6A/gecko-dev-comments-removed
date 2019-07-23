




































var gTestfile = 'regress-372309.js';

var BUGNUMBER = 372309;
var summary = 'Root new array objects';
var actual = 'No Crash';
var expect = 'No Crash';

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var width = 600;
  var height = 600;

  var img1canvas = document.createElement("canvas");
  var img2canvas = document.createElement("canvas");

  img1canvas.width = img2canvas.width = width;
  img1canvas.height = img2canvas.height = height;
  img1canvas.getContext("2d").getImageData(0, 0, width, height).data;
  img2canvas.getContext("2d").getImageData(0, 0, width, height).data;
 
  reportCompare(expect, actual, summary);
  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

  exitFunc ('test');
}

if (typeof window != 'undefined')
{
  
  gDelayTestDriverEnd = true;

  window.addEventListener("load", test, false);
}
else
{
  reportCompare(expect, actual, summary);
}

