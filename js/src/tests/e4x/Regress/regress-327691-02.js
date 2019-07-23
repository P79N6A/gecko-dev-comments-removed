





































gTestfile = 'regress-327691-02.js';

var summary = "Do not crash during gc()";
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

gc();

TEST(1, expect, actual);

END();
