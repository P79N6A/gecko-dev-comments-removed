





































var gTestfile = 'regress-387871.js';


var BUGNUMBER = 387871;
var summary = 'Do not assert: gen->state != JSGEN_RUNNING && gen->state != JSGEN_CLOSING';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var a = gen();

  try {
    a.next();
    throw "a.next() should throw about already invoked generator";
  } catch (e) {
    if (!(e instanceof TypeError))
      throw e;
  }

  function gen()
  {
    for (x in a)
      yield 1;
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
