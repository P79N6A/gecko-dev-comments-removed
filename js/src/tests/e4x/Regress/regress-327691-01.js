





































gTestfile = 'regress-327691-01.js';

var summary = "Do not crash in js_IsXMLName";
var BUGNUMBER = 327691;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var A=<y/>;
var B=A.h;
var D=B.h;
B.h=D.h;
B[0]=B.h;

B.h=D.h; 

TEST(1, expect, actual);

END();
