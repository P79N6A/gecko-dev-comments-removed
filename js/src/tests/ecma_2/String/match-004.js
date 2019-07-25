









































var SECTION = "String/match-004.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.match( regexp )";

var BUGNUMBER="http://scopus/bugsplat/show_bug.cgi?id=345818";

startTest();


re = /0./;
s = 10203040506070809000;

Number.prototype.match = String.prototype.match;

AddRegExpCases(  re,
		 "re = " + re ,
		 s,
		 String(s),
		 1,
		 ["02"]);


re.lastIndex = 0;
AddRegExpCases(  re,
		 "re = " + re +" [lastIndex is " + re.lastIndex+"]",
		 s,
		 String(s),
		 1,
		 ["02"]);





























test();

function AddRegExpCases(
  regexp, str_regexp, string, str_string, index, matches_array ) {

  

  if ( regexp.exec(string) == null || matches_array == null ) {
    AddTestCase(
      string + ".match(" + regexp +")",
      matches_array,
      string.match(regexp) );

    return;
  }

  AddTestCase(
    "( " + string  + " ).match(" + str_regexp +").length",
    matches_array.length,
    string.match(regexp).length );

  AddTestCase(
    "( " + string + " ).match(" + str_regexp +").index",
    index,
    string.match(regexp).index );

  AddTestCase(
    "( " + string + " ).match(" + str_regexp +").input",
    str_string,
    string.match(regexp).input );

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

function AddGlobalRegExpCases(
  regexp, str_regexp, string, matches_array ) {

  

  if ( regexp.exec(string) == null || matches_array == null ) {
    AddTestCase(
      regexp + ".exec(" + string +")",
      matches_array,
      regexp.exec(string) );

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
