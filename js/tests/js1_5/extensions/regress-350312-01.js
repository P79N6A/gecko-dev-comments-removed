




































var bug = 350312;
var summary = 'Accessing wrong stack slot with nested catch/finally';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var tmp;

  function f()
    {
      try {   
        try {   
          throw 1;
        } catch (e) {
          throw e;
        } finally {
          tmp = true;
        }
      } catch (e) {
        return e;
      }
    }

  var ex = f();

  var passed = ex === 1;
  if (!passed) {
    print("Failed!");
    print("ex="+uneval(ex));
  }
  reportCompare(true, passed, summary);

  exitFunc ('test');
}
