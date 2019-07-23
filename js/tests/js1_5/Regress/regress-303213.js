




































var gTestfile = 'regress-303213.js';

var BUGNUMBER = 303213;
var summary = 'integer overflow in js';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
printStatus('This bug passes if no crash occurs');

expectExitCode(5);

var s=String.fromCharCode(257);

var ki="";
var me="";
for (i = 0; i < 1024; i++)
{
  ki = ki + s;
}

for (i = 0; i < 1024; i++)
{
  me = me + ki;
}

var ov = s;

for (i = 0; i < 28; i++)
  ov += ov;

for (i = 0; i < 88; i++)
  ov += me;

printStatus("done generating");
var eov = escape(ov);
printStatus("done escape");
printStatus(eov);
 
reportCompare(expect, actual, summary);
