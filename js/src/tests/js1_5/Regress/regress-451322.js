





































var BUGNUMBER = 451322;
var summary = 'Do not crash with OOM in LirBufWriter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  function f() {
    for (var i = 0; i < 200000; i++) {
      var m = new Function("var k = 0; for (var j = 0; j < 5; j++) { k += j * 2 + 8 / (j+3) * k} return k;");
      m();
    }
  }
  f();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
