





































gTestfile = 'function-002.js';




























var SECTION = "function-002.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 325843";
var BUGNUMBER="330462";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

dec1 = "function f1(x,y){++x, --y}";
dec2 = "function f2(){var y; f1(1,2); y=new Date(); print(y.toString())}";

eval(dec1);
eval(dec2);

new TestCase(
  SECTION,
  "typeof f1",
  "function",
  typeof f1 );



new TestCase(
  SECTION,
  "f1.toString() == dec1",
  true,
  StripSpaces(f1.toString()) == StripSpaces(dec1));

new TestCase(
  SECTION,
  "typeof f2",
  "function",
  typeof f2 );



new TestCase(
  SECTION,
  "f2.toString() == dec2",
  true,
  StripSpaces(f2.toString().replace(/new Date\(\)/g, 'new Date')) ==
  StripSpaces(dec2.replace(/new Date\(\)/g, 'new Date')));

test();

function StripSpaces( s ) {
  var strippedString = "";
  for ( var currentChar = 0; currentChar < s.length; currentChar++ ) {
    if (!IsWhiteSpace(s.charAt(currentChar))) {
      strippedString += s.charAt(currentChar);
    }
  }
  return strippedString;
}

function IsWhiteSpace( string ) {
  var cc = string.charCodeAt(0);

  switch (cc) {
  case (0x0009):
  case (0x000B):
  case (0x000C):
  case (0x0020):
  case (0x000A):
  case (0x000D):
  case ( 59 ): 
    return true;
    break;
  default:
    return false;
  }
}

