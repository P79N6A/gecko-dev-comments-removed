






































var bug = 374163;
var summary = 'Set E4X xml.function::__proto__ = null causes toString to throw';
var actual = '';
var expect = 'TypeError: String.prototype.toString called on incompatible XML';

printBugNumber (bug);
printStatus (summary);

try
{
    var a = <a/>; 
    a.function::__proto__ = null; 
    "" + a;
}
catch(ex)
{
    actual = ex + '';
}

TEST(1, expect, actual);
END();
