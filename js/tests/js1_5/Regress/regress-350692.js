




































var gTestfile = 'regress-350692.js';

var BUGNUMBER = 350692;
var summary = 'import x["y"]["z"]';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var x = {y: {z: function() {}}};

  try
  {
    import x['y']['z'];
  }
  catch(ex)
  {
    reportCompare('TypeError: x["y"]["z"] is not exported', ex + '', summary);
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
