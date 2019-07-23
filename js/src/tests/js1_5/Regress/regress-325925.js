






var gTestfile = 'regress-325925.js';

var BUGNUMBER = 325925;
var summary = 'Do not Assert: c <= cs->length in AddCharacterToCharSet';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

/[\cA]/('\x01');
 
reportCompare(expect, actual, summary);
