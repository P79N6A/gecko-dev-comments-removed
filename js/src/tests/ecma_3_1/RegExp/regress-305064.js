




































var gTestfile = 'regress-305064.js';

var BUGNUMBER = 305064;
var summary = 'CharacterClassEscape \\s';
var actual = '';
var expect = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  var whitespace = [
    {s : '\u0009', t : 'HORIZONTAL TAB'},
    {s : '\u000B', t : 'VERTICAL TAB'},
    {s : '\u000C', t : 'FORMFEED'},
    {s : '\u0020', t : 'SPACE'},
    {s : '\u00A0', t : 'NO-BREAK SPACE'},
    {s : '\u1680', t : 'OGHAM SPACE MARK'},
    {s : '\u180E', t : 'MONGOLIAN VOWEL SEPARATOR'},
    {s : '\u2000', t : 'EN QUAD'},
    {s : '\u2001', t : 'EM QUAD'},
    {s : '\u2002', t : 'EN SPACE'},
    {s : '\u2003', t : 'EM SPACE'},
    {s : '\u2004', t : 'THREE-PER-EM SPACE'},
    {s : '\u2005', t : 'FOUR-PER-EM SPACE'},
    {s : '\u2006', t : 'SIX-PER-EM SPACE'},
    {s : '\u2007', t : 'FIGURE SPACE'},
    {s : '\u2008', t : 'PUNCTUATION SPACE'},
    {s : '\u2009', t : 'THIN SPACE'},
    {s : '\u200A', t : 'HAIR SPACE'},
    {s : '\u202F', t : 'NARROW NO-BREAK SPACE'},
    {s : '\u205F', t : 'MEDIUM MATHEMATICAL SPACE'},
    {s : '\u3000', t : 'IDEOGRAPHIC SPACE'},
    {s : '\u000A', t : 'LINE FEED OR NEW LINE'},
    {s : '\u000D', t : 'CARRIAGE RETURN'},
    {s : '\u2028', t : 'LINE SEPARATOR'},
    {s : '\u2029', t : 'PARAGRAPH SEPARATOR'},
    {s : '\u200B', t : 'ZERO WIDTH SPACE (category Cf)'}
    ];

  for (var i = 0; i < whitespace.length; ++i)
  {
    var v = whitespace[i];
    reportCompare(true, !!(/\s/.test(v.s)), 'Is ' + v.t + ' a space');
  }
 
  exitFunc ('test');
}
