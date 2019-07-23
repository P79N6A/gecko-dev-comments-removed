





































var bug = 245308;
var summary = 'Don\'t Crash with nest try catch';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

function foo(e) {
  try {}
  catch(ex) {
    try {}
    catch(ex) {}
  }
}

foo('foo');

actual = 'No Crash';
  
reportCompare(expect, actual, summary);
