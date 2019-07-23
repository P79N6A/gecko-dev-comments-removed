







































var gTestfile = 'regress-28686.js';

test();

function test()
{   
  enterFunc ("test");

  printBugNumber (28686);
   
  var str = 'foo "bar" baz';
  reportCompare ('foo \\"bar\\" baz', str.replace(/([\'\"])/g, "\\$1"),
		 "str.replace failed.");
   
  exitFunc ("test");
   
}
