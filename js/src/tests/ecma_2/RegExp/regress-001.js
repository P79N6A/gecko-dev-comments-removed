















var SECTION = "RegExp/hex-001.js";
var VERSION = "ECMA_2";
var TITLE   = "JS regexp anchoring on empty match bug";
var BUGNUMBER = "2157";

startTest();

AddRegExpCases( /a||b/.exec(''),
		"/a||b/.exec('')",
		1,
		[''] );

test();

function AddRegExpCases( regexp, str_regexp, length, matches_array ) {

  AddTestCase(
    "( " + str_regexp + " ).length",
    regexp.length,
    regexp.length );


  for ( var matches = 0; matches < matches_array.length; matches++ ) {
    AddTestCase(
      "( " + str_regexp + " )[" + matches +"]",
      matches_array[matches],
      regexp[matches] );
  }
}
