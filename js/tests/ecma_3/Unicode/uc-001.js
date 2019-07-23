







































var gTestfile = 'uc-001.js';

test();

function test()
{
  enterFunc ("test");

  printStatus ("Unicode format-control character (Category Cf) test.");
  printBugNumber (23610);

  reportCompare ("no error", eval('"no\u200E error"'),
		 "Unicode format-control character test (Category Cf.)");
   
  exitFunc ("test");
}
