




































var gTestfile = 'regress-378492.js';

var BUGNUMBER = 378492;
var summary = 'namespace_trace/qname_trace should check for null private, ' +
  'WAY_TOO_MUCH_GC';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  options('xml');

  x = <x/>;
  for each(x.t in x) { }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
