




































var gTestfile = 'regress-367120-01.js';

var BUGNUMBER = 367120;
var summary = 'memory corruption in script_toSource';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  if (typeof Script == 'undefined' || !('toSource' in {}))
  {
    print('Test skipped. Script or toSource not defined');
  }
  else
  {
    var s = new Script("");
    var o = {
      valueOf : function() {
        s.compile("");
        Array(11).join(Array(11).join(Array(101).join("aaaaa")));
        return;
      }
    };
    s.toSource(o);
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
