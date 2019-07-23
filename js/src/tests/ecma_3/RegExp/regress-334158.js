




































var gTestfile = 'regress-334158.js';

var BUGNUMBER = 334158;
var summary = 'Parse error in control letter escapes (RegExp)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = true;
actual = /\ca/.test( "\x01" ); 
reportCompare(expect, actual, summary + ':/\ca/.test( "\x01" )');

expect = false
  actual = /\ca/.test( "\\ca" );
reportCompare(expect, actual, summary + ': /\ca/.test( "\\ca" )');

expect = false
  actual = /\c[a/]/.test( "\x1ba/]" );
reportCompare(expect, actual, summary + ': /\c[a/]/.test( "\x1ba/]" )');
