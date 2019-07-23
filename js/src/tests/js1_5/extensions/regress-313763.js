




































var gTestfile = 'regress-313763.js';

var BUGNUMBER = 313763;
var summary = 'Root jsarray.c creatures';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
printStatus ('This bug requires TOO_MUCH_GC');

var N = 0x80000002;
var array = Array(N);
array[N - 1] = 1;
array[N - 2] = 2;


var LOOP_TERMINATOR = "stop_long_loop";
array.__defineGetter__(N - 2, function() {
			 throw "stop_long_loop";
		       });

var prepared_string = String(1);
array.__defineGetter__(N - 1, function() {
			 var tmp = prepared_string;
			 prepared_string = null;
			 return tmp;
		       })


  try {
    array.unshift(1);
  } catch (e) {
    if (e !== LOOP_TERMINATOR)
      throw e;
  }

var expect = "1";
var actual = array[N];
printStatus(expect === actual);

reportCompare(expect, actual, summary);
