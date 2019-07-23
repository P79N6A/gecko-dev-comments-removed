





































var gTestfile = 'regress-476192.js';

var BUGNUMBER = 476192;
var summary = 'TM: Do not assert: JSVAL_TAG(v) == JSVAL_STRING';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var global;

  (function(){
    var ad = {present: ""};
    var params = ['present', 'a', 'present', 'a', 'present', 'a', 'present'];
    for (var j = 0; j < params.length; j++) {
      global = ad[params[j]];
    }
  })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
