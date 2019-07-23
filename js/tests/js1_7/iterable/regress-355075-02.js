




































var bug = 355075;
var summary = 'Regression tests from bug 354750';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var jsOptions = new JavaScriptOptions();


  jsOptions.setOption('strict', true);
  jsOptions.setOption('werror', true);

  function f() {
    this.a = <><a/><b/></>
      var dummy;
    for (var b in this.a)
      dummy = b;
  }

  f();
  
  jsOptions.reset();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
