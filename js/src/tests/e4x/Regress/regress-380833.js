





































gTestfile = 'regress-380833.js';

var summary = "Crash during GC after uneval";
var BUGNUMBER = 380833;
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
START(summary);

var www = <x><y/></x>; 
print(uneval(this) + "\n"); 
gc();

TEST(1, expect, actual);

END();
