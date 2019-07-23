




































var gTestfile = 'regress-315990.js';

var BUGNUMBER = 315990;
var summary = 'this.statement.is.an.error';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary + ': function expression');

expect = 'TypeError';
try
{
  (function() {
    this.statement.is.an.error;
  })()
    }
catch(ex)
{
  printStatus(ex);
  actual = ex.name;
} 
reportCompare(expect, actual, summary + ': function expression');


printStatus (summary + ': top level');
try
{
  this.statement.is.an.error;
}
catch(ex)
{
  printStatus(ex);
  actual = ex.name;
} 
reportCompare(expect, actual, summary + ': top level');
