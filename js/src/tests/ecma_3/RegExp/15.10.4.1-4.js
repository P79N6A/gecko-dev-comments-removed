











































































var gTestfile = '15.10.4.1-4.js';
var BUGNUMBER = '61266';
var summary = 'Passing a RegExp object to a RegExp() constructor';
var statprefix = 'Applying RegExp() twice to pattern ';
var statmiddle = ' and flag ';
var statsuffix =  '; testing property ';
var singlequote = "'";
var i = -1; var j = -1; var s = '';
var obj1 = {}; var obj2 = {};
var status = ''; var actual = ''; var expect = ''; var msg = '';
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
      obj1 = new RegExp(s, f);
      obj2 = new RegExp(obj1, undefined);  
 
      reportCompare (obj1 + '', obj2 + '', status);
    }
  }

  exitFunc ('test');
}


function getStatus(regexp, flag)
{
  return (statprefix  +  quote(regexp) +  statmiddle  +  flag  +  statsuffix);
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}
