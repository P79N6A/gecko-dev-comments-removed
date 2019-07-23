








































































var gTestfile = '15.10.3.1-2.js';
var BUGNUMBER = '61266';
var summary = 'Passing (RegExp object,flag) to RegExp() function';
var statprefix = 'RegExp(new RegExp(';
var comma =  ', '; var singlequote = "'"; var closeparens = '))';
var cnSUCCESS = 'RegExp() returned the supplied RegExp object';
var cnFAILURE =  'RegExp() did NOT return the supplied RegExp object';
var i = -1; var j = -1; var s = ''; var f = '';
var obj = {};
var status = ''; var actual = ''; var expect = '';
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
flags[3] = undefined;




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
      status = getStatus(s, f);
      obj = new RegExp(s, f);  

      actual = (obj == RegExp(obj, undefined))? cnSUCCESS : cnFAILURE ;
      expect = cnSUCCESS;
      reportCompare (expect, actual, status);
    }
  }
 
  exitFunc ('test');
}


function getStatus(regexp, flag)
{
  return (statprefix  +  quote(regexp) +  comma  +   flag  +  closeparens);
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}
