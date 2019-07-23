





































gTestfile = 'regress-328249.js';

var summary = "Crash due to infinite recursion in js_IsXMLName";
var BUGNUMBER = 328249;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

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
