




































var SECTION = "String/match-003.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.match( regexp )";

startTest();















re = /([\d]{5})([-\ ]?[\d]{4})?$/g;


s = "Boston, MA 02134";

AddGlobalRegExpCases( re,
		      "re = " + re,
		      s,
		      ["02134" ]);

re.lastIndex = 0;

AddGlobalRegExpCases(
  re,
  "re = " + re + "; re.lastIndex = 0 ",
  s,
  ["02134"]);


re.lastIndex = s.length;

AddGlobalRegExpCases(
  re,
  "re = " + re + "; re.lastIndex = " + s.length,
  s,
  ["02134"] );

re.lastIndex = s.lastIndexOf("0");

AddGlobalRegExpCases(
  re,
  "re = "+ re +"; re.lastIndex = " + s.lastIndexOf("0"),
  s,
  ["02134"]);

re.lastIndex = s.lastIndexOf("0") + 1;

AddGlobalRegExpCases(
  re,
  "re = " +re+ "; re.lastIndex = " + (s.lastIndexOf("0") +1),
  s,
  ["02134"]);

test();

function AddGlobalRegExpCases(
  regexp, str_regexp, string, matches_array ) {

  

  if ( string.match(regexp) == null || matches_array == null ) {
    AddTestCase(
      string + ".match(" + str_regexp +")",
      matches_array,
      string.match(regexp) );

    return;
  }

  AddTestCase(
    "( " + string  + " ).match(" + str_regexp +").length",
    matches_array.length,
    string.match(regexp).length );

  var limit = matches_array.length > string.match(regexp).length ?
    matches_array.length :
    string.match(regexp).length;

  for ( var matches = 0; matches < limit; matches++ ) {
    AddTestCase(
      "( " + string + " ).match(" + str_regexp +")[" + matches +"]",
      matches_array[matches],
      string.match(regexp)[matches] );
  }
}
