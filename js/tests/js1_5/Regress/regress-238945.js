





































var bug = 238945;
var summary = '7.9.1 Automatic Semicolon insertion - allow token following do{}while()';
var actual = 'error';
var expect = 'no error';

printBugNumber (bug);
printStatus (summary);

function f(){do;while(0)return}f()
  
actual = 'no error'
reportCompare(expect, actual, summary);
