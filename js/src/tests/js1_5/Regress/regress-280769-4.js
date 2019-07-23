





































var gTestfile = 'regress-280769-4.js';

var BUGNUMBER = 280769;
var summary = 'Do not overflow 64K length of char sequence in RegExp []';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);


var N = 20 * 1000; 

var prefixes = ["000", "00", "0"];

function to_4_hex(i)
{
  var printed = i.toString(16);
  if (printed.length < 4) {
    printed= prefixes[printed.length - 1]+printed;
  }
  return printed;

}

var a = new Array(N);
for (var i = 0; i != N; ++i) {
  a[i] = to_4_hex(2*i);
}

var str = '[\\u'+a.join('\\u')+']';


var re = new RegExp(str);

var string_to_match = String.fromCharCode(2 * (N - 1));

var value = re.exec(string_to_match);

var expect =  string_to_match;
var actual = value ? value[0] : value;

reportCompare(expect, actual, summary);
