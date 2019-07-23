




































var gTestfile = 'regress-476869.js';

var BUGNUMBER = 476869;
var summary = 'Do not assert: v != JSVAL_ERROR_COOKIE';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof gczeal == 'undefined')
  {
    gczeal = (function (){});
  }

  jit(true);

  function f()
  {
    (new Function("gczeal(1); for each (let y in [/x/,'',new Boolean(false),new Boolean(false),new Boolean(false),'',/x/,new Boolean(false),new Boolean(false)]){}"))();
  }
  __proto__.__iterator__ = this.__defineGetter__("", function(){})
    f();

  jit(false);

  delete __proto__.__iterator__;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
