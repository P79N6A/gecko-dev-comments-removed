




































var gTestfile = 'regress-367119-02.js';

var BUGNUMBER = 367119;
var summary = 'memory corruption in script_exec';
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
      valueOf : function() {
        s.compile("");
        print(1);
        return {};
      }
    };
    s.exec(o);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
