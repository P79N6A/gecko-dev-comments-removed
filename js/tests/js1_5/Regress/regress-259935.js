




































var bug = 259935;
var summary = 'document.all can be easily detected';
var actual = '';
var expect = 'not detected';

printBugNumber (bug);
printStatus (summary);

if (typeof document == 'undefined')
{
  document = {};
}

function foo() {
    this.ie = document.all;
}

var f = new foo();

if (f.ie) {
    actual = 'detected';
} else {
    actual = 'not detected';
}
  
reportCompare(expect, actual, summary);

f = {ie: document.all};

if (f.ie) {
    actual = 'detected';
} else {
    actual = 'not detected';
}
  
reportCompare(expect, actual, summary);
