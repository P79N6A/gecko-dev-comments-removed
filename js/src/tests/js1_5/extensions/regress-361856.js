




































var gTestfile = 'regress-361856.js';

var BUGNUMBER = 361856;
var summary = 'Do not assert: overwriting @ js_AddScopeProperty';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function testit() {
    var obj = {};
    obj.watch("foo", function(){});
    delete obj.foo;
    obj = null;
    gc();
  }
  testit();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
