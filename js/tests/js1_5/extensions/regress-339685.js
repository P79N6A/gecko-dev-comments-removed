




































var bug = 339685;
var summary = 'Setting __proto__ null should not affect __iterator__';
var actual = '';
var expect = 'No Error';

printBugNumber (bug);
printStatus (summary);
  
var d = { a:2, b:3 };

d.__proto__ = null;

try { 
  for (var p in d)
    ;
  actual = 'No Error';
} catch(e) { 
  actual = e + ''; 
}

reportCompare(expect, actual, summary);
