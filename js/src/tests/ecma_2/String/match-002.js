








































var SECTION = "String/match-002.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.match( regexp )";

startTest();




AddRegExpCases( /([\d]{5})([-\ ]?[\d]{4})?$/,
		"/([\d]{5})([-\ ]?[\d]{4})?$/",
		"Boston, Mass. 02134",
		14,
		["02134", "02134", undefined]);

AddGlobalRegExpCases( /([\d]{5})([-\ ]?[\d]{4})?$/g,
		      "/([\d]{5})([-\ ]?[\d]{4})?$/g",
		      "Boston, Mass. 02134",
		      ["02134"]);


re = /([\d]{5})([-\ ]?[\d]{4})?$/;
re.lastIndex = 0;

s = "Boston, MA 02134";

AddRegExpCases( re,
		"re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex =0",
		s,
		s.lastIndexOf("0"),
		["02134", "02134", undefined]);


re.lastIndex = s.length;

AddRegExpCases( re,
		"re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
		s.length,
		s,
		s.lastIndexOf("0"),
		["02134", "02134", undefined] );

re.lastIndex = s.lastIndexOf("0");

AddRegExpCases( re,
		"re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
		s.lastIndexOf("0"),
		s,
		s.lastIndexOf("0"),
		["02134", "02134", undefined]);

re.lastIndex = s.lastIndexOf("0") + 1;

AddRegExpCases( re,
		"re = /([\d]{5})([-\ ]?[\d]{4})?$/; re.lastIndex = " +
		s.lastIndexOf("0") +1,
		s,
		s.lastIndexOf("0"),
		["02134", "02134", undefined]);

test();

function AddRegExpCases(
  regexp, str_regexp, string, index, matches_array ) {

  

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
    string,
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
