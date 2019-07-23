




































var gTestfile = 'regress-465013.js';

var BUGNUMBER = 465013;
var summary = '';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'bgcolor="dummy" quality="dummy" allowScriptAccess="dummy" ';

  jit(true);

  print((function(x) {
        var ja = "";
        var ka = {bgcolor:"#FFFFFF", quality:"high", allowScriptAccess:"always"};
        for (var la in ka) {
          ja +=[la] + "=\"" + x + "\" ";
        }
        return actual = ja;
      })("dummy"));

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
