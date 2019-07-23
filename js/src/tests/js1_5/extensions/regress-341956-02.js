




































var gTestfile = 'regress-341956-02.js';

var BUGNUMBER = 341956;
var summary = 'GC Hazards in jsarray.c - pop';
var actual = '';
var expect = 'GETTER RESULT';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var N = 0xFFFFFFFF;
  var a = []; 
  a[N - 1] = 0;

  var expected = "GETTER RESULT";

  a.__defineGetter__(N - 1, function() {
		       delete a[N - 1];
		       var tmp = [];
		       tmp[N - 2] = 1;

		       if (typeof gc == 'function')
			 gc();
		       for (var i = 0; i != 50000; ++i) {
			 var tmp = 1 / 3;
			 tmp /= 10;
		       }
		       for (var i = 0; i != 1000; ++i) {
			 
			 
			 
			 var tmp2 = Array(12).join(' ');
		       }
		       return expected;
		     });

  actual = a.pop();

  reportCompare(expect, actual, summary);

  print('Done');

  exitFunc ('test');
}
