





































var gTestfile = '15.10.4.1-5-n.js';





























var BUGNUMBER = '61266';
var summary = 'Negative test: Passing (RegExp object, flag) to RegExp() constructor';
var statprefix = 'Passing RegExp object on pattern ';
var statsuffix =  '; passing flag ';
var cnFAILURE = 'Expected an exception to be thrown, but none was -';
var singlequote = "'";
var i = -1; var j = -1; var s = ''; var f = '';
var obj1 = {}; var obj2 = {};
var patterns = new Array();
var flags = new Array(); 



patterns[0] = '';
patterns[1] = 'abc';
patterns[2] = '(.*)(3-1)\s\w';
patterns[3] = '(.*)(...)\\s\\w';
patterns[4] = '[^A-Za-z0-9_]';
patterns[5] = '[^\f\n\r\t\v](123.5)([4 - 8]$)';


flags[0] = 'i';
flags[1] = 'g';
flags[2] = 'm';


DESCRIPTION = "Negative test: Passing (RegExp object, flag) to RegExp() constructor"
  EXPECTED = "error";



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (i in patterns)
  {
    s = patterns[i];

    for (j in flags)
    {
      f = flags[j];
      printStatus(getStatus(s, f));
      obj1 = new RegExp(s, f);  
      obj2 = new RegExp(obj1, f);   

      
      reportCompare('PASS', 'FAIL', cnFAILURE);
    }
  }

  exitFunc ('test');
}


function getStatus(regexp, flag)
{
  return (statprefix  +  quote(regexp) +  statsuffix  +   flag);
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}
