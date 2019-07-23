




































var bug = 344262;
var summary = 'Variables bound by let statement/expression';
var actual = '';
var expect = 0;



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  function f() {
    var a = [];
    for (var i = 0; i < 10; i++) {
      a[i] = let (j = i) function () { return j; };
      var b = [];
      for (var k = 0; k <= i; k++)
        b.push(a[k]());
      print(b.join());
    }
    actual = a[0]();
    print(actual);
  }
  f();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
