




































var bug = 361856;
var summary = 'Assertion: overwriting @ js_AddScopeProperty';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
