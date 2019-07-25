





































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

  var doc;
  if (typeof document == 'undefined')
  {
    doc = {};
  }
  else
  {
    doc = document;
  }

  if (typeof alert == 'undefined')
  {
    alert = print;
  }


  doc.watch("title", function(a,b,c,d) {
		   return { toString : function() { alert(1); } };
		 });
  doc.title = "xxx";


  doc.watch("title", function() {
		   return { toString : function() { alert(1); } };
		 });
  doc.title = "xxx";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
