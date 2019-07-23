




































var gTestfile = 'regress-336100.js';

var BUGNUMBER = 336100;
var summary = 'bug 336100 - arguments regressed';
var actual = '';
var expect;

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = '[object Object]';
actual = (function(){return (arguments + '');})(); 
reportCompare(expect, actual, summary);


expect = typeof window == 'undefined' ? '' : '[object Object]';
actual = (function(){with (this) return(arguments + '');})();
reportCompare(expect, actual, summary);
