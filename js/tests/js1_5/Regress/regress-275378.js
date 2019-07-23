







































var bug = 275378;
var summary = 'Literal RegExp in case block should not give syntax error';
var actual = '';
var expect = '';

var status;

printBugNumber (bug);
printStatus (summary);


var tmpString= "XYZ";

/ABC/.test(tmpString);
var tmpVal= 1;
if (tmpVal == 1)
{
	
	/ABC/.test(tmpString);
}
switch(tmpVal)
{
case 1:
	{
		
		/ABC/.test(tmpString);
	}
	break;
}
switch(tmpVal)
{
case 1:
	
	/ABC/.test(tmpString);
	break;
}

expect = actual = 'no error';
reportCompare(expect, actual, status);
