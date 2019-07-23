




































var gTestfile = 'regress-446026-01.js';

var BUGNUMBER = 446026;
var summary = 'brian loves eval(s, o)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var b = 45;


  var obj = (function() {
      var a = 21;
      return {
        
      fn: function() {a;}
      };
    })();

expect = 'ReferenceError: a is not defined | undefined | 45';
actual = '';

var foo;

try {
  eval('bar = b; foo=a', obj.fn);
} catch (e) {
  actual = e;
}
print(actual += " | " + foo + " | " + bar); 
reportCompare(expect, actual, summary);

expect = 'No Error';
actual = 'No Error';

try
{
  eval("", {print:1});
  print(1);
}
catch(ex)
{
  actual = ex + '';
}
reportCompare(expect, actual, summary);
