





































var BUGNUMBER = 341956;
var summary = 'GC Hazards in jsarray.c - reverse';
var actual = 'No Crash';
var expect = 'No Crash';


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



  var toStop = "stringToStop";
  a[N - 3] = 0;
  a.__defineGetter__(N - 3, function() { throw toStop; });


  var good = false;

  try {
    a.reverse();
  } catch (e) {
    if (e === toStop)
      good = true;
  }

  expect = true;
  actual = good;

  reportCompare(expect, actual, summary);

  print('Done');

  exitFunc ('test');
}
