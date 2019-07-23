




































var gTestfile = 'regress-504078.js';

var BUGNUMBER = 504078;
var summary = 'Iterations over global object';
var actual = '';
var expect = '';

var g = (typeof window == 'undefined' ? this : window);


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function keys(obj) {
    for (var prop in obj) {
    }
  }

  var data = { a : 1, b : 2 };
  var data2 = { a : 1, b : 2 };

  function boot() {
	  keys(data);
    keys(g);
    keys(data2); 
    print('no error');
  }

  try
  {
    boot();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
