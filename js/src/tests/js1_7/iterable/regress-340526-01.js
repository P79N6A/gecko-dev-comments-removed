




































var gTestfile = 'regress-340526-01.js';

var BUGNUMBER = 340526;
var summary = 'Iterators: cross-referenced objects with close handler can ' +
  'delay close handler execution';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  var iter = Iterator({});
  iter.foo = "bar";
  for (var i in iter)
    ;
}
catch(ex)
{
  print(ex + '');
}
 
reportCompare(expect, actual, summary);
