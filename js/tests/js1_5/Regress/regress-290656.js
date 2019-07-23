




































var bug = 290656;
var summary = 'Regression from bug 254974';
var actual = 'No Error';
var expect = 'No Error';

printBugNumber (bug);
printStatus (summary);

function foo() {
  with(foo) {
    this["insert"] = function(){ var node = new bar(); };
  }
  function bar() {}
}

try
{
  var list = new foo();
  list.insert();
}
catch(e)
{
  actual = e + '';
}
 
reportCompare(expect, actual, summary);
