




































var gTestfile = 'regress-479740.js';

var BUGNUMBER = 479740;
var summary = 'TM: Do not crash @ TraceRecorder::test_property_cache';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

try
{
  eval(
    '  function f() {' +
    '    for ( foobar in (function() {' +
    '          for (var x = 0; x < 2; ++x) {' +
    '            if (x % 2 == 1) { yield (this.z.z) = function(){} }' +
    '            eval("this", false);' +
    '          }' +
    '        })());' +
    '      function(){}' +
    '  }' +
    '  f();'
    );
}
catch(ex)
{
}
jit(false);


reportCompare(expect, actual, summary);
