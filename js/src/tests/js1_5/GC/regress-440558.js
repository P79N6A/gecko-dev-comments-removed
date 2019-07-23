




































var gTestfile = 'regress-440558.js';

var BUGNUMBER = 440558;
var summary = 'Do not assert: *flagp != GCF_FINAL';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print('Note: this test requires that javascript.options.gczeal ' +
        'be set to 2 prior to the browser start');

  m = function(a, b) {
    if (++i < 10) {
    }
  };
  e = function(a, b) {
  };

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

