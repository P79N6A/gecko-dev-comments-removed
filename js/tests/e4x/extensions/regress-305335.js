




































START("Regression - XML instance methods should type check in JS_GetPrivate()");

var bug = 305335;
var summary = 'XML instance methods should type check in JS_GetPrivate()';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var o = new Number(0); 
o.__proto__ = XML(); 

try
{  
    o.parent();
}
catch(e)
{
    printStatus('Exception: ' + e);
}

TEST(1, expect, actual);
END();
