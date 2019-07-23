




































var gTestfile = 'regress-361964.js';

var BUGNUMBER = 361964;
var summary = 'Crash [@ MarkGCThingChildren] involving watch and setter';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof document == 'undefined')
  {
    document = {};
  }

  if (typeof alert == 'undefined')
  {
    alert = print;
  }


  document.watch("title", function(a,b,c,d) {
		   return { toString : function() { alert(1); } };
		 });
  document.title = "xxx";


  document.watch("title", function() {
		   return { toString : function() { alert(1); } };
		 });
  document.title = "xxx";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
