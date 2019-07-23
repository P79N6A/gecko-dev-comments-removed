




































var bug = 365527;
var summary = 'JSOP_ARGUMENTS should set obj register';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  

  counter = 500*1000;

  var obj;

  function getter()
    {
      obj = { get x() { 
        return getter(); 
      }, counter: counter};
      return obj;
    }


  var x;

  function g()
    {
      x += this.counter;
      if (--counter == 0)
        throw "Done";
    }


  function f()
    {
      arguments=g;
      try {
        for (;;) {
          arguments();
          obj.x;
        }
      } catch (e) {
        print(e);
      }
    }


  getter();
  f();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
