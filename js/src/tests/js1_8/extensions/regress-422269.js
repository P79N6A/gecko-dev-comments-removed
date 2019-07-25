





































var BUGNUMBER = 422269;
var summary = 'Compile-time let block should not capture runtime references';
var actual = 'referenced only by stack and closure';
var expect = 'referenced only by stack and closure';



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  function f()
  {
    let m = {sin: Math.sin};
    (function holder() { m.sin(1); })();
    return m;
  }

  if (typeof findReferences == 'undefined')
  {
    expect = actual = 'Test skipped';
    print('Test skipped. Requires findReferences function.');
  }
  else
  {
    var x = f();
    var refs = findReferences(x);

    
    
    
    
    
    for (var edge in refs) {
      
      if (refs[edge].every(function (r) r === null))
        delete refs[edge];
      
      
      else if (refs[edge].length === 1 &&
               typeof refs[edge][0] === "function" &&
               refs[edge][0].name === "holder")
        delete refs[edge];
    }

    if (Object.keys(refs).length != 0)
        actual = "unexpected references to the result of f: " + Object.keys(refs).join(", ");
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
