




































var bug = 310607;
var summary = 'Do not crash iterating over Object.prototype';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var f = new Foo();
f.go("bar");

function Foo() {
  this.go = function(prototype) {
    printStatus("Start");
    for(var i in Object.prototype) {
      printStatus("Here");
      eval("5+4");
    }
    printStatus("End");
  };
}
  
reportCompare(expect, actual, summary);
