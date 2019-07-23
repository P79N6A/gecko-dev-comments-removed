







































var gTestfile = 'uc-002-n.js';

DESCRIPTION = "Non-character escapes in identifiers negative test.";
EXPECTED = "error";

enterFunc ("test");

printStatus ("Non-character escapes in identifiers negative test.");
printBugNumber (23607);

eval("\u0020 = 5");
reportCompare('PASS', 'FAIL', "Previous statement should have thrown an error.");

exitFunc ("test");

