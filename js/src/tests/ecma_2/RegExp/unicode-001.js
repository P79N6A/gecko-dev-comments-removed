













var SECTION = "RegExp/unicode-001.js";
var VERSION = "ECMA_2";
var TITLE   = "new RegExp( pattern, flags )";

startTest();



AddRegExpCases( /\u0041/, "/\\u0041/",   "A", "A", 1, 0, ["A"] );
AddRegExpCases( /\u00412/, "/\\u00412/", "A2", "A2", 1, 0, ["A2"] );
AddRegExpCases( /\u00412/, "/\\u00412/", "A2", "A2", 1, 0, ["A2"] );
AddRegExpCases( /\u001g/, "/\\u001g/", "u001g", "u001g", 1, 0, ["u001g"] );

AddRegExpCases( /A/,  "/A/",  "\u0041", "\\u0041",   1, 0, ["A"] );
AddRegExpCases( /A/,  "/A/",  "\u00412", "\\u00412", 1, 0, ["A"] );
AddRegExpCases( /A2/, "/A2/", "\u00412", "\\u00412", 1, 0, ["A2"]);
AddRegExpCases( /A/,  "/A/",  "A2",      "A2",       1, 0, ["A"] );

test();

function AddRegExpCases(
  regexp, str_regexp, pattern, str_pattern, length, index, matches_array ) {

  AddTestCase(
    str_regexp + " .exec(" + str_pattern +").length",
    length,
    regexp.exec(pattern).length );

  AddTestCase(
    str_regexp + " .exec(" + str_pattern +").index",
    index,
    regexp.exec(pattern).index );

  AddTestCase(
    str_regexp + " .exec(" + str_pattern +").input",
    pattern,
    regexp.exec(pattern).input );

  for ( var matches = 0; matches < matches_array.length; matches++ ) {
    AddTestCase(
      str_regexp + " .exec(" + str_pattern +")[" + matches +"]",
      matches_array[matches],
      regexp.exec(pattern)[matches] );
  }
}
