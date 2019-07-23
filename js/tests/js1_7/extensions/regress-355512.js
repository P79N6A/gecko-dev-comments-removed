




































var bug = 355512;
var summary = 'Do not crash with generator arguments';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  function foopy()
    {
      var f = function(){ r = arguments; d.d.d; yield 170; }
      try { for (var i in f()) { } } catch (iterError) { }   
    }

  typeof uneval;
  foopy();
  gc();
  uneval(r);
  gc();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
