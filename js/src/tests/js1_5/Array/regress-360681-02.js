




































var gTestfile = 'regress-360681-02.js';

var BUGNUMBER = 360681;
var summary = 'Regression from bug 224128';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = actual = 'No Crash';

  var N = 1000;


  var a = Array(N);
  for (i = 0; i < N - 1; ++i)
    a[i] = 1;









  var str1 = Array(2*(2*N + 1) + 1).join(String.fromCharCode(0xFFF0));
  var str2 = Array(4*(2*N + 1) + 1).join(String.fromCharCode(0xFFF0));
  gc();
  str1 = str2 = null;
  gc();

  var firstCall = true;
  a.sort(function (a, b) {
	   if (firstCall) {
	     firstCall = false;
	     gc();
	   }
	   return a - b;
	 });

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
