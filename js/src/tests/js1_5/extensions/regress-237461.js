





































var gTestfile = 'regress-237461.js';

var BUGNUMBER = 237461;
var summary = 'don\'t crash with nested function collides with var';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

function g()
{
  var core = {};
  core.js = {};
  core.js.init = function()
    {
      var loader = null;
       
      function loader() {}
    };
  return core;
}

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
}
else
{
  var s = new Script(""+g.toString());
  try
  {
    var frozen = s.freeze(); 
    printStatus("len:" + frozen.length);
  }
  catch(e)
  {
  }
}
actual = 'No Crash';

reportCompare(expect, actual, summary);
