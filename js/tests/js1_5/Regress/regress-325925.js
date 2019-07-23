




































var bug = 325925;
var summary = 'Do not Assert: c <= cs->length in AddCharacterToCharSet';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

/[\cA]/('\x01');
  
reportCompare(expect, actual, summary);
