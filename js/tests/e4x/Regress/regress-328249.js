





































START("Crash due to infinite recursion in js_IsXMLName");

var bug = 327897;
var summary = 'Crash due to infinite recursion in js_IsXMLName';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{
    var A = <x/>;
    var B = A.p1;
    var C = B.p2;
    B.p3 = C;
    C.p4 = B;
    C.appendChild(B);
    C.p5 = C;
}
catch(ex)
{
    printStatus(ex+'');
}
TEST(1, expect, actual);

END();
