




































var bug = 306738;
var summary = 'uneval() on objects with getter or setter';
var actual = '';
var expect = '';

printBugNumber (bug);
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
