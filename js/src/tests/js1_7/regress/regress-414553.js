




































var gTestfile = 'regress-414553.js';

var BUGNUMBER = 414553;
printBugNumber(BUGNUMBER);
printStatus(summary);

var expected = '1,2,3,4';
var actual = let (a = 1, [b,c] = [2,3], d = 4) ( String([a,b,c,d]) );

reportCompare(expected, actual, 'destructuring assignment in let');

function f() {
  let (a = 1, [b,c] = [2,3], d = 4) {
    return String([a,b,c,d]);
  }
}

reportCompare(expected, f(), 'destructuring assignment in let inside func');
