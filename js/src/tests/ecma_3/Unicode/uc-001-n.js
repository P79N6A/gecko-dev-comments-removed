







































var gTestfile = 'uc-001-n.js';

test();

function test()
{
  enterFunc ("test");

  printStatus ("Unicode Characters 1C-1F negative test.");
  printBugNumber (23612);
   
  reportCompare ("error", eval ("'no'\u001C+' error'"),
		 "Unicode whitespace test (1C.)");
  reportCompare ("error", eval ("'no'\u001D+' error'"),
		 "Unicode whitespace test (1D.)");
  reportCompare ("error", eval ("'no'\u001E+' error'"),
		 "Unicode whitespace test (1E.)");
  reportCompare ("error", eval ("'no'\u001F+' error'"),
		 "Unicode whitespace test (1F.)");

  exitFunc ("test");
}
