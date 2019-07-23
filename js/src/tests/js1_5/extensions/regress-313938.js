




































var gTestfile = 'regress-313938.js';

var BUGNUMBER = 313938;
var summary = 'Root access in jsscript.c';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
  reportCompare("Script not defined, Test skipped.",
                "Script not defined, Test skipped.",
                summary);
}
else
{
  var str = " 2;".substring(1);
  "1".substring(2);
  expect = Script.prototype.compile(str).toSource();

  var likeString = {
    toString: function() {
      var tmp = str;
      str = null;
      return tmp;
    }
  };

  TWO = 2.0;

  var likeObject = {
    valueOf: function() {
      if (typeof gc == "function")
        gc();
      for (var i = 0; i != 40000; ++i) {
        var tmp = 1e100 * TWO;
      }
      return this;
    }
  }

  var s = Script.prototype.compile(likeString, likeObject);
  var actual = s.toSource();
  printStatus(expect === actual);

  reportCompare(expect, actual, summary);
}
