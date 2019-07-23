





































gTestfile = 'continue.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'statements: continue';

writeHeaderToLog("Executing script: continue.js");
writeHeaderToLog( SECTION + " "+ TITLE);

var i,j;

j = 0;
for (i = 0; i < 200; i++)
{
  if (i == 100)
    continue;
  j++;
}


new TestCase ( SECTION, '"continue" in "for" loop',
	       199, j);


j = 0;
out1:
for (i = 0; i < 1000; i++)
{
  if (i == 100)
  {
  out2:
    for (var k = 0; k < 1000; k++)
    {
      if (k == 500) continue out1;
    }
    j = 3000;
  }
  j++;
}


new TestCase ( SECTION, '"continue" in "for" loop with a "label"',
	       999, j);

i = 0;
j = 1;

while (i != j)
{
  i++;
  if (i == 100) continue;
  j++;
}


new TestCase ( SECTION, '"continue" in a "while" loop',
	       100, j );

j = 0;
i = 0;
out3:
while (i < 1000)
{
  if (i == 100)
  {
    var k = 0;
  out4:
    while (k < 1000)
    {
      if (k == 500)
      {
	i++;
	continue out3;
      }
      k++;
    }
    j = 3000;
  }
  j++;
  i++;
}


new TestCase ( SECTION, '"continue" in a "while" loop with a "label"',
	       999, j);

i = 0;
j = 1;

do
{
  i++;
  if (i == 100) continue;
  j++;
} while (i != j);



new TestCase ( SECTION, '"continue" in a "do" loop',
	       100, j );

j = 0;
i = 0;
out5:
do
{
  if (i == 100)
  {
    var k = 0;
  out6:
    do
    {
      if (k == 500)
      {
	i++;
	continue out5;
      }
      k++;
    }while (k < 1000);
    j = 3000;
  }
  j++;
  i++;
}while (i < 1000);


new TestCase ( SECTION, '"continue" in a "do" loop with a "label"',
	       999, j);

test();
