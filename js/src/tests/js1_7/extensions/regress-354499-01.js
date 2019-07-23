




































var gTestfile = 'regress-354499-01.js';

var BUGNUMBER = 354499;
var summary = 'Iterating over Array elements';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = actual = 'No Crash';

  var obj = {get a(){ return new Object(); }};

  function setter(v)
  {
    
    var tmp = { get toString() { return new Object(); }};
    try { String(tmp); } catch (e) {  }
    gc();
  }

  Array.prototype.__defineGetter__(0, function() { });
  Array.prototype.__defineSetter__(0, setter);

  for (var i in Iterator(obj))
    print(uneval(i));

  delete Array.prototype[0];

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
