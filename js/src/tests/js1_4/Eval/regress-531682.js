





var gTestfile = 'regress-531682.js';

var BUGNUMBER = 531682;
var summary = 'Checking proper wrapping of scope in  eval(source, scope)';
var actual;
var expect;


var x = 0;

test();


function scope1() {
    eval('var x = 1;');
    return function() { return x; }
}

function test() {
    enterFunc ('test');
    printBugNumber(BUGNUMBER);
    printStatus (summary);

    
    actual = eval('x', scope1());
    expect = 0;
    reportCompare(expect, actual, summary);
    exitFunc ('test');
}
