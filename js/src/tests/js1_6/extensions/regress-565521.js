








var BUGNUMBER = 565521;
var summary = 'for-each getter calling';
var actual = '';
var expect = 'PASS';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var proto = { get prop() { return (this === proto) ? "FAIL" : "PASS"; } };
  var obj = Object.create(proto);

  var itercount = 0;
  for each (var i in obj) {
      ++itercount;
      actual = i;
  }

  reportCompare(itercount, 1, summary + ': right itercount')
  reportCompare(expect, actual, summary + ': right value');

  exitFunc ('test');
}

