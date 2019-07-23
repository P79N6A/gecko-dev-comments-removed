




































var gTestfile = 'regress-346773.js';

var BUGNUMBER = 346773;
var summary = 'Do not crash compiling with misplaced brances in function';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    var src =
      '    var it = {foo:"5"};' +
      '    it.__iterator__ =' +
      '      function(valsOnly)' +
      '      {' +
      '        var gen =' +
      '        function()' +
      '        {' +
      '          for (var i = 0; i < keys.length; i++)' +
      '          {' +
      '            if (valsOnly)' +
      '              yield vals[i];' +
      '            else' +
      '              yield [keys[i], vals[i]];' +
      '          }' +
      '          return gen();' +
      '        }' +
      '      }';
    eval(src);
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
