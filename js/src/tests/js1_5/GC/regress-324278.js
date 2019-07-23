




































var gTestfile = 'regress-324278.js';

var BUGNUMBER = 324278;
var summary = 'GC without recursion';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);




var N = 100*1000;

function build(N) {
  
  
  
  

  var chainTop = null;
  for (var i = 0; i != N; ++i) {
    var f = Function('some_arg'+i, ' return /test/;');
    var re = f();
    re.previous = chainTop;
    chainTop = f;
  }
  return chainTop;
}

function check(chainTop, N) {
  for (var i = 0; i != N; ++i) {
    var re = chainTop();
    chainTop = re.previous;
  }
  if (chainTop !== null)
    throw "Bad chainTop";

}

if (typeof gc != "function") {
  gc = function() {
    for (var i = 0; i != 50*1000; ++i) {
      var tmp = new Object();
    }
  }
}

var chainTop = build(N);
printStatus("BUILT");
gc();
check(chainTop, N);
printStatus("CHECKED");
chainTop = null;
gc();
 
reportCompare(expect, actual, summary);
