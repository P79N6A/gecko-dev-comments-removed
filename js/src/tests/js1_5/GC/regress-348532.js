




































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
    try {
        return err.apply(this, arguments);
    } catch (e) {
        if (!(e instanceof InternalError))
            throw e;
    }
    return new Error();
  }

  
  
  var error = err(s,s,s,s);

  print(error.stack.length);

  expect = true;
  actual = (error.stack.length > 0);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
