






































var bug = 346794;
var summary = 'Do not crash';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  
  expectExitCode(0);
  expectExitCode(3);

  function boo() {
    s = '';
    for (;;) {
      try {
        new RegExp(s + '[\\');
      } catch(e) {}
      s += 'q';
    }
  }

  boo();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
