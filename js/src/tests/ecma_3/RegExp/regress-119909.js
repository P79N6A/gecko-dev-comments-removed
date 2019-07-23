













































var gTestfile = 'regress-119909.js';
var BUGNUMBER = 119909;
var summary = "Shouldn't crash on regexps with many nested parentheses";
var NO_BACKREFS = false;
var DO_BACKREFS = true;



test();



function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  testThis(500, NO_BACKREFS, 'hello', 'goodbye');
  testThis(500, DO_BACKREFS, 'hello', 'goodbye');

  reportCompare('No Crash', 'No Crash', '');

  exitFunc('test');
}






function testThis(numParens, doBackRefs, strOriginal, strReplace)
{
  var openParen = doBackRefs? '(' : '(?:';
  var closeParen = ')';
  var pattern = '';
 
  for (var i=0; i<numParens; i++) {pattern += openParen;}
  pattern += strOriginal;
  for (i=0; i<numParens; i++) {pattern += closeParen;}
  var re = new RegExp(pattern);

  var res = strOriginal.search(re);
  res = strOriginal.match(re);
  res = strOriginal.replace(re, strReplace);
}
