




































var gTestfile = 'regress-306738.js';

var BUGNUMBER = 306738;
var summary = 'uneval() on objects with getter or setter';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

actual = uneval(
  {
    get foo()
    {
      return "foo";
    }
  });

expect = '({get foo() {return "foo";}})';
 
compareSource(expect, actual, summary);
