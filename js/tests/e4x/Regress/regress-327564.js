






































START("Hang due to cycle in XML object");

var bug = 327564;
var summary = 'Hang due to cycle in XML object';
var actual = 'No Cycle Detected';
var expect = 'Error: cyclic XML value';

printBugNumber (bug);
printStatus (summary);

var p = <p/>;

p.c = 1;

var c = p.c[0];

p.insertChildBefore(null,c);

printStatus(p.toXMLString());

printStatus('p.c[1] === c');
TEST(1, true, p.c[1] === c);

p.c = 2

try
{
    c.appendChild(p)
    
}
catch(ex)
{
    actual = ex + '';
    printStatus(actual);
}

TEST(2, expect, actual);

END();
