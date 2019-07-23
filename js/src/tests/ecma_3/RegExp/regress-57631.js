
















































var gTestfile = 'regress-57631.js';
var BUGNUMBER = '57631';
var summary = 'Testing new RegExp(pattern,flag) with illegal pattern or flag';
var statprefix = 'Testing for error creating illegal RegExp object on pattern ';
var statsuffix =  'and flag ';
var cnSUCCESS = 'SyntaxError';
var cnFAILURE = 'not a SyntaxError';
var singlequote = "'";
var i = -1; var j = -1; var s = ''; var f = '';
var obj = {};
var status = ''; var actual = ''; var expect = ''; var msg = '';
var legalpatterns = new Array(); var illegalpatterns = new Array();
var legalflags = new Array();  var illegalflags = new Array();



legalpatterns[0] = '';
legalpatterns[1] = 'abc';
legalpatterns[2] = '(.*)(3-1)\s\w';
legalpatterns[3] = '(.*)(...)\\s\\w';
legalpatterns[4] = '[^A-Za-z0-9_]';
legalpatterns[5] = '[^\f\n\r\t\v](123.5)([4 - 8]$)';


illegalpatterns[0] = '(?)';
illegalpatterns[1] = '(a';
illegalpatterns[2] = '( ]';



legalflags[0] = 'i';
legalflags[1] = 'g';
legalflags[2] = 'm';
legalflags[3] = undefined;


illegalflags[0] = 'a';
illegalflags[1] = 123;
illegalflags[2] = new RegExp();




test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  testIllegalRegExps(legalpatterns, illegalflags);
  testIllegalRegExps(illegalpatterns, legalflags);
  testIllegalRegExps(illegalpatterns, illegalflags);

  exitFunc ('test');
}



function testIllegalRegExps(patterns, flags)
{
  for (i in patterns)
  {
    s = patterns[i];
 
    for (j in flags)
    {
      f = flags[j];
      status = getStatus(s, f);
      actual = cnFAILURE;
      expect = cnSUCCESS;
 
      try
      {
	
	eval('obj = new RegExp(s, f);'); 
      } 
      catch(e)
      {
	
	if (e instanceof SyntaxError)
	  actual = cnSUCCESS;
      }
       
      reportCompare(expect, actual, status);
    }
  }
}


function getStatus(regexp, flag)
{
  return (statprefix  +  quote(regexp) +  statsuffix  +   quote(flag));
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}
