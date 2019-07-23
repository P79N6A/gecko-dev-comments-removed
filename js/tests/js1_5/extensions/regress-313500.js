




































var gTestfile = 'regress-313500.js';

var BUGNUMBER = 313500;
var summary = 'Root access to "prototype" property';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
printStatus('This test requires TOO_MUCH_GC');

function F() { }

var prepared = new Object();

F.prototype = {};
F.__defineGetter__('prototype', function() {
		     var tmp = prepared;
		     prepared = null;
		     return tmp;
		   });

new F();
 
reportCompare(expect, actual, summary);
