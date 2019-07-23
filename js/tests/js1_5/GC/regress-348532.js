




































var gTestfile = 'regress-348532.js';

var BUGNUMBER = 348532;
var summary = 'Do not overflow int when constructing Error.stack';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expectExitCode(0);
  expectExitCode(3);

  actual = 0;
 
  
  var s = Array((1<<23)+1).join('x');

  var recursionDepth = 0;
  function err() {
    if (++recursionDepth == 128)
      return new Error();
    return err.apply(this, arguments);
  }

  
  
  var error = err(s,s);

  print(error.stack.length);

  expect = true;
  actual = (error.stack.length > 0);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
