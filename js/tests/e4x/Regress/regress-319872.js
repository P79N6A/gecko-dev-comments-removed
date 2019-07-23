




































var bug = 319872;
var summary = 'Do not Crash in jsxml.c';
var actual = 'No Crash';
var expect = 'No Crash';


printBugNumber (bug);
printStatus (summary);
printStatus ("Expect out of memory error");
expectExitCode(3);

var i,m,str;
str="<a xmlns:v=\"";
m="";

for (i=0;i<(1024*1024)/2;i++) 
  m += "\n";

for(i=0;i<500;i++) 
  str += m ;

str += "\">f00k</a>";

var xx = new XML(str);

printStatus(xx.toXMLString());
  
TEST(1, expect, actual);
