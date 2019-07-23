




































var gTestfile = 'regress-351070-03.js';

var BUGNUMBER = 351070;
var summary = 'decompilation of let declaration should not change scope';
var summarytrunk = 'let declaration must be direct child of block or top-level implicit block';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var c;
  var f;

  try
  {
    c = '(function (x){if (x) if (y) z; else let w })';
    f = eval(c);
    expect = 'function (x){if (x) {if (y) { z; } else let w; }}';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function (x){if (x){ if (y) z;} else let w  })';
    f = eval(c);
    expect = 'function (x){if (x){ if (y) {z;}} else let w;  }';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function (x){if (x){ if (y) let z;} else let w  })';
    f = eval(c);
    expect = 'function (x){if (x){ if (y) let z;} else let w;  }';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function f(){var a = 2; if (x) {let a = 3; print(a)} return a})';
    f = eval(c);
    expect = 'function f(){var a = 2; if (x) {let a = 3; print(a);} return a;}';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function f(){var a = 2; if (x) {print(a);let a = 3} return a})';
    f = eval(c);
    expect = 'function f(){var a = 2; if (x) {print(a);let a = 3;} return a;}';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function f(){var a = 2; if (x) {let a = 3} return a})';
    f = eval(c);
    expect = 'function f(){var a = 2; if (x) {let a = 3;} return a;}';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  try
  {
    c = '(function f(){var a = 2; if (x) let a = 3; return a})';
    f = eval(c);
    expect = 'function f(){var a = 2; if (x) let a = 3; return a;}';
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    
    expect = 'SyntaxError';
    actual = ex.name;
    reportCompare(expect, actual, summarytrunk + ': ' + c);
  }

  exitFunc ('test');
}
