




































var bug = 350793;
var summary = 'for-in loops must be yieldable';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  function foo()
    {
      function gen() { for(let itty in [5,6,7,8]) yield ({ }); }

      iter = gen();

      let count = 0;
      for each(let _ in iter)
                ++count;
    }

  foo();
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
