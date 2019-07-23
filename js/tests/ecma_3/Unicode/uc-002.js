







































var gTestfile = 'uc-002.js';

test();

function test()
{
  enterFunc ("test");

  printStatus ("Unicode non-breaking space character test.");
  printBugNumber (23613);

  reportCompare ("no error", eval("'no'\u00A0+ ' error'"),
		 "Unicode non-breaking space character test.");

  var str = "\u00A0foo";
  reportCompare (0, str.search(/^\sfoo$/),
		 "Unicode non-breaking space character regexp test.");

  exitFunc ("test");
}
