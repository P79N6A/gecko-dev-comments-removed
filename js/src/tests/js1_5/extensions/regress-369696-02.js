




































var gTestfile = 'regress-369696-02.js';

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

  native = encodeURIComponent;
  n = native.prototype;
  n.__defineGetter__("prototype", n.toSource);
  p = n.__lookupGetter__("prototype");
  n = p;
  n["prototype"] = [n];
  n = p;
  p2 = n["prototype"];
  n = p2;
  n.__defineGetter__("0", n.toString);
  n = p;
  n();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
