




































var bug = 352797;
var summary = 'Assertion: OBJ_GET_CLASS(cx, obj) == &js_BlockClass';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  if (typeof Script == 'undefined')
  {
    print('Test skipped. Script not defined.');
  }
  else
  {
    (function(){let x = 'fafafa'.replace(/a/g, new Script(''))})();
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
