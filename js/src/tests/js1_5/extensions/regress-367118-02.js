




































var gTestfile = 'regress-367118-02.js';

var BUGNUMBER = 367118;
var summary = 'memory corruption in script_compile';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof Script == 'undefined')
  {
    print('Test skipped. Script or toSource not defined');
  }
  else
  {
    var s = new Script("");
    var o = {
      toString : function() {
        s.compile("");
        print(1);
        return "a";
      }
    };
    s.compile(o);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
