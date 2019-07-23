





































START("Do not crash in js_IsXMLName");

var bug = 327691;
var summary = 'Do not crash in js_IsXMLName';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var A=<y/>;
var B=A.h;
var D=B.h;
B.h=D.h;
B[0]=B.h;

B.h=D.h; 

TEST(1, expect, actual);

END();
