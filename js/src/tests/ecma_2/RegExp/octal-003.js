






























var SECTION = "RegExp/octal-003.js";
var VERSION = "ECMA_2";
var TITLE   = "RegExp patterns that contain OctalEscapeSequences";
var BUGNUMBER="http://scopus/bugsplat/show_bug.cgi?id=346132";

startTest();

AddRegExpCases( /.\011/, "/\\011/", "a" + String.fromCharCode(0) + "11", "a\\011", 0, null );

test();

function AddRegExpCases(
  regexp, str_regexp, pattern, str_pattern, index, matches_array ) {

  

  if ( regexp.exec(pattern) == null || matches_array == null ) {
    AddTestCase(
      regexp + ".exec(" + str_pattern +")",
      matches_array,
      regexp.exec(pattern) );

    return;
  }
  AddTestCase(
    str_regexp + ".exec(" + str_pattern +").length",
    matches_array.length,
    regexp.exec(pattern).length );

  AddTestCase(
    str_regexp + ".exec(" + str_pattern +").index",
    index,
    regexp.exec(pattern).index );

  AddTestCase(
    str_regexp + ".exec(" + str_pattern +").input",
    escape(pattern),
    escape(regexp.exec(pattern).input) );

  AddTestCase(
    str_regexp + ".exec(" + str_pattern +").toString()",
    matches_array.toString(),
    escape(regexp.exec(pattern).toString()) );

  var limit = matches_array.length > regexp.exec(pattern).length
    ? matches_array.length
    : regexp.exec(pattern).length;

  for ( var matches = 0; matches < limit; matches++ ) {
    AddTestCase(
      str_regexp + ".exec(" + str_pattern +")[" + matches +"]",
      matches_array[matches],
      escape(regexp.exec(pattern)[matches]) );
  }

}
