




































var gTestfile = 'regress-369696-03.js';

var BUGNUMBER = 396696;
var summary = 'Do not assert: map->depth > 0" in js_LeaveSharpObject';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var x = [[[ { toSource: function() { gc();  }}]]];

  var a = [];
  a[0] = a;
  a.toSource = a.toString;
  Array.prototype.toSource.call(a);



  (function() {
    var tmp = [];
    for (var i = 0; i != 30*1000; ++i) {
      var tmp2 = [];
      tmp.push(tmp2);
      tmp2.toSource();
    }
  })();

  gc();
  x.toSource();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
