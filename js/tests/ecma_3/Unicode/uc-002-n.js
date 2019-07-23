







































DESCRIPTION = "Non-character escapes in identifiers negative test.";
EXPECTED = "error";

enterFunc ("test");

printStatus ("Non-character escapes in identifiers negative test.");
printBugNumber (23607);

eval("\u0020 = 5");
reportFailure("Previous statement should have thrown an error.");

exitFunc ("test");

