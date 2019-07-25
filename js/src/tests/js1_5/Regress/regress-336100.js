





































var BUGNUMBER = 336100;
var summary = 'bug 336100 - arguments regressed';
var actual = '';
var expect;

printBugNumber(BUGNUMBER);
printStatus (summary);

var arguments = [];

expect = '[object Arguments]';
actual = (function(){return (arguments + '');})(); 
reportCompare(expect, actual, summary);


expect = '';
actual = (function(){with (this) return(arguments + '');})();
reportCompare(expect, actual, summary);
