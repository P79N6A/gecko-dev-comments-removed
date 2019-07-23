





































gTestfile = 'function-003.js';










var SECTION = "toString-001.js";
var VERSION = "JS1_4";
var TITLE   = "Regression test case for 104766";
var BUGNUMBER="310514";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(
  SECTION,
  "StripSpaces(Array.prototype.concat.toString()).substring(0,17)",
  "functionconcat(){",
  StripSpaces(Array.prototype.concat.toString()).substring(0,17));

test();

function StripSpaces( s ) {
  for ( var currentChar = 0, strippedString="";
        currentChar < s.length; currentChar++ )
  {
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

