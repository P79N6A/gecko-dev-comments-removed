




































var gTestfile = 'regress-375183.js';

var BUGNUMBER = 375183;
var summary = '__noSuchMethod__ should not allocate beyond fp->script->depth';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj = { get __noSuchMethod__() {
      print("Executed");
      return new Object();
    }};

  try
  {
    obj.x();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary + ':1');

  obj = { __noSuchMethod__: {} };
  try
  {
    obj.x();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary + ':2');

  obj = { }
  obj.__noSuchMethod__ = {};
  try
  {
    obj.x();
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary + ':3');

  exitFunc ('test');
}
