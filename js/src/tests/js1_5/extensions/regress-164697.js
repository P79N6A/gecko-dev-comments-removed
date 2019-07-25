





































var BUGNUMBER = 164697;
var summary = '(parent(instance) == parent(constructor))';
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

runtest('new String()', 'String');

runtest('new Boolean()', 'Boolean');

runtest('new Number("1")', 'Number');

runtest('new Date()', 'Date');

runtest('/x/', 'RegExp');
runtest('new RegExp("x")', 'RegExp');

runtest('new Error()', 'Error');

function runtest(myinstance, myconstructor)
{
  var expr;
  var actual;

  if (typeof parent === "function")
  {
    try
    {
      expr =
        'parent(' + myinstance + ') == ' +
        'parent(' + myconstructor + ')';
      printStatus(expr);
      actual = eval(expr).toString();
    }
    catch(ex)
    {
      actual = ex + '';
    }

    reportCompare(expect, actual, expr);
  }

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
