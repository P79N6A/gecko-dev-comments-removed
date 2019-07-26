






var BUGNUMBER = 476257;
var summary = 'Do not assert: !JS_ON_TRACE(cx)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
jit(true);

function f1() {
  try
  {
    Object.defineProperty(__proto__, "functional",
    {
      enumerable: true, configurable: true,
      get: function ()
      {
        if (typeof gczeal == 'function') { gczeal(0); }
      }
    });
    for each (let [[]] in [true, new Boolean(true), new Boolean(true)]) {}
  }
  catch(ex)
  {
  }
}

function f2() {
  try
  {
    Object.defineProperty(__proto__, "functional",
    {
      enumerable: true, configurable: true,
      get: function () 
      { 
        if (typeof dis == 'function') { dis(); } 
      }
    });
    for each (let [[]] in [true, new Boolean(true), new Boolean(true)]) {}
  }
  catch(ex)
  {
  }
}

f1();
f2();

jit(false);

reportCompare(expect, actual, summary);
