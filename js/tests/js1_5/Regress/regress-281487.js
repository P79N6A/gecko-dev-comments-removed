




































var bug = 281487;
var summary = 'JSOP_ARGDEC assertion when tracing';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

printStatus('This test requires a DEBUG build and will cause a false ' +
            'failure to be reported by jsDriver.pl since the tracing output ' +
            'will contain the string FAILED.');
printStatus('This test only fails if it causes a crash.');

if (typeof tracing == 'function')
{
  tracing(true);
}

var x;

var a = function (i,j,k) {
  x = j--;
};

a(1,2,3);
if (typeof tracing == 'function')
{
  tracing(false);
}  
reportCompare(expect, actual, summary);
