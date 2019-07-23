




































var gTestfile = 'regress-466905-03.js';

var BUGNUMBER = 466905;
var summary = 'decompile anonymous functions returning arrays';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'function () {\n' +
    'return [];\n' + 
    '},function () {\n' + 
    'return [1];\n' + 
    '},function () {\n' + 
    'return [1, ];\n' +
    '},function () {\n' + 
    'return [1, 2];\n' + 
    '},function () {\n' + 
    'return [1, , 3];\n' + 
    '},function () {\n' + 
    'return [1, , 3, ];\n' + 
    '},function () {\n' + 
    'return [(a, b)];\n' + 
    '},function () {\n' + 
    'return foo((a, b));\n' + 
    '}';
  actual = ([                                
    function() { return []; },
    function() { return [1]; },
    function() { return [1, ]; },
    function() { return [1, 2]; },
    function() { return [1, , 3]; },
    function() { return [1, , 3, ]; },
    function() { return [(a, b)]; },
    function() { return foo((a, b)); },
              ]) + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
