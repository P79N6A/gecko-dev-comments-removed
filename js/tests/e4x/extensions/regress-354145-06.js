





































gTestfile = 'regress-354145-06.js';

var BUGNUMBER = 354145;
var summary = 'Immutable XML';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);


var list = <><a/><b/></>

function a() {
   delete list[1];
   delete list[0];
   gc();
   for (var i = 0; i != 50000; ++i)
     var tmp = ""+i;    
  return true;
}

var list2 = list.(a());

print(uneval(list2))

TEST(1, expect, actual);

END();
