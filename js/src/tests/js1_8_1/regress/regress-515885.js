

var gTestfile = 'regress-515885.js';
var it = (x for (x in [function(){}]));
it.next();

reportCompare("no assertion failure", "no assertion failure", "See bug 515885.");
