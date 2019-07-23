





































gTestfile = 'break.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'statements: break';

writeHeaderToLog("Executing script: break.js");
writeHeaderToLog( SECTION + " "+ TITLE);

var i,j;

for (i = 0; i < 1000; i++)
{
  if (i == 100) break;
}


new TestCase ( SECTION, 'breaking out of "for" loop',
	       100, i);

j = 2000;

out1:
for (i = 0; i < 1000; i++)
{
  if (i == 100)
  {
  out2:
    for (j = 0; j < 1000; j++)
    {
      if (j == 500) break out1;
    }
    j = 2001;
  }
  j = 2002;
}


new TestCase ( SECTION, 'breaking out of a "for" loop with a "label"',
	       500, j);

i = 0;

while (i < 1000)
{
  if (i == 100) break;
  i++;
}


new TestCase ( SECTION, 'breaking out of a "while" loop',
	       100, i );


j = 2000;
i = 0;

out3:
while (i < 1000)
{
  if (i == 100)
  {
    j = 0;
  out4:
    while (j < 1000)
    {
      if (j == 500) break out3;
      j++;
    }
    j = 2001;
  }
  j = 2002;
  i++;
}


new TestCase ( SECTION, 'breaking out of a "while" loop with a "label"',
	       500, j);

i = 0;

do
{
  if (i == 100) break;
  i++;
} while (i < 1000);


new TestCase ( SECTION, 'breaking out of a "do" loop',
	       100, i );

j = 2000;
i = 0;

out5:
do
{
  if (i == 100)
  {
    j = 0;
  out6:
    do
    {
      if (j == 500) break out5;
      j++;
    }while (j < 1000);
    j = 2001;
  }
  j = 2002;
  i++;
}while (i < 1000);


new TestCase ( SECTION, 'breaking out of a "do" loop with a "label"',
	       500, j);

test();
