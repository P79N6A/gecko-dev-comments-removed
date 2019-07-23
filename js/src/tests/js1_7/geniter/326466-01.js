




































var gTestfile = '326466-01.js';

var BUGNUMBER = 326466;
var summary = 'Implement Pythonic generators and iteration protocol support';
var actual;
var expect;

printBugNumber(BUGNUMBER);
printStatus (summary);
 
function fib()
{
  var a = 0, b = 1;

  while (true)
  {
    yield a;
    var t = a;
    a = b;
    b += t;
  }
}

var g = fib();

expect = '[object Generator]';
actual = g.toString();
reportCompare(expect, actual, summary);

var actual = [];
var expect = [0, 1, 1, 2, 3, 5, 8, 13];
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
actual.push(g.next());
reportCompare(expect.join(), actual.join(), summary);

