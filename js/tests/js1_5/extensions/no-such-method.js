





































var gTestfile = 'no-such-method.js';

var BUGNUMBER = 196097;
var summary = '__noSuchMethod__ handler';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var o = {
  __noSuchMethod__: function (id, args)
  {
    return(id + '('+args.join(',')+')');
  }
};

status = summary + ' ' + inSection(1) + ' ';
actual = o.foo(1,2,3);
expect = 'foo(1,2,3)';
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + ' ';
actual = o.bar(4,5);
expect = 'bar(4,5)';
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3) + ' ';
actual = o.baz();
expect = 'baz()';
reportCompare(expect, actual, status);
