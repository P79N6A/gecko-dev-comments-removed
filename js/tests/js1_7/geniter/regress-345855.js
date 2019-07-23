




































var bug = 345855;
var summary = 'Blank yield expressions are not syntax errors';
var actual = '';
var expect = 'No Error';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  expect = 'SyntaxError: syntax error';
  try
  {
    eval('function() {x = 12 + yield;}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function() {x = 12 + yield;}');

  expect = 'SyntaxError: yield expression must be parenthesized';
  try
  {
    eval('function () {foo(yield)}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function () {foo(yield)}');

  expect = 'SyntaxError: syntax error';
  try
  {
    eval('function() {x = 12 + yield 42}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function() {x = 12 + yield 42}');

  expect = 'SyntaxError: yield expression must be parenthesized';
  try
  {
    eval('function (){foo(yield 42)}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function (){foo(yield 42)}');

  expect = 'No Error';

  try
  {
    eval('function() {x = 12 + (yield);}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function() {x = 12 + (yield);}');

  try
  {
    eval('function () {foo((yield))}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function () {foo((yield))}');

  try
  {
    eval('function() {x = 12 + (yield 42)}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function() {x = 12 + (yield 42)}');

  try
  {
    eval('function (){foo((yield 42))}');
    actual = 'No Error';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary + ': function (){foo((yield 42))}');

  exitFunc ('test');
}
