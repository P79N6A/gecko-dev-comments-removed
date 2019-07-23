




































var gTestfile = 'regress-164697.js';

var BUGNUMBER = 164697;
var summary = '(instance.__parent__ == constructor.__parent__)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = 'true';

runtest('{}', 'Object');
runtest('new Object()', 'Object');



actual = (function (){}).__proto__ == Function.prototype;
reportCompare('true', actual+'',
              '(function (){}).__proto__ == Function.prototype');

runtest('new Function(";")', 'Function');

runtest('[]', 'Array');
runtest('new Array()', 'Array');

runtest('""', 'String');
runtest('new String()', 'String');

runtest('true', 'Boolean');
runtest('new Boolean()', 'Boolean');

runtest('1', 'Number');
runtest('new Number("1")', 'Number');

runtest('new Date()', 'Date');

runtest('/x/', 'RegExp');
runtest('new RegExp("x")', 'RegExp');

runtest('new Error()', 'Error');

function runtest(myinstance, myconstructor)
{
  var expr;
  var actual;

  try
  {
    expr =  '(' + myinstance + ').__parent__ == ' +
      myconstructor + '.__parent__';
    printStatus(expr);
    actual = eval(expr).toString();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, expr);

  try
  {
    expr =  '(' + myinstance + ').__proto__ == ' +
      myconstructor + '.prototype';
    printStatus(expr);
    actual = eval(expr).toString();
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, expr);
}
