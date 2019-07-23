







































var gTestfile = 'uc-003.js';

test();

function test()
{
  enterFunc ("test");

  var \u0041 = 5;
  var A\u03B2 = 15;
  var c\u0061se = 25;

  printStatus ("Escapes in identifiers test.");
  printBugNumber (23608);
  printBugNumber (23607);

  reportCompare (5, eval("\u0041"),
		 "Escaped ASCII Identifier test.");
  reportCompare (6, eval("++\u0041"),
		 "Escaped ASCII Identifier test");
  reportCompare (15, eval("A\u03B2"),
		 "Escaped non-ASCII Identifier test");
  reportCompare (16, eval("++A\u03B2"),
		 "Escaped non-ASCII Identifier test");
  reportCompare (25, eval("c\\u00" + "61se"),
		 "Escaped keyword Identifier test");
  reportCompare (26, eval("++c\\u00" + "61se"),
		 "Escaped keyword Identifier test");
   
  exitFunc ("test");
}
