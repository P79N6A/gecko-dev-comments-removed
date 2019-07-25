




































var SECTION = "String/match-001.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.match( regexp )";

startTest();






AddRegExpCases( 3, "3",   "1234567890", 1, 2, ["3"] );



AddGlobalRegExpCases( /34/g, "/34/g", "343443444",  3, ["34", "34", "34"] );
AddGlobalRegExpCases( /\d{1}/g,  "/d{1}/g",  "123456abcde7890", 10,
		      ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"] );

AddGlobalRegExpCases( /\d{2}/g,  "/d{2}/g",  "123456abcde7890", 5,
		      ["12", "34", "56", "78", "90"] );

AddGlobalRegExpCases( /\D{2}/g,  "/d{2}/g",  "123456abcde7890", 2,
		      ["ab", "cd"] );

test();


function AddRegExpCases(
  regexp, str_regexp, string, length, index, matches_array ) {

  AddTestCase(
    "( " + string  + " ).match(" + str_regexp +").length",
    length,
    string.match(regexp).length );

  AddTestCase(
    "( " + string + " ).match(" + str_regexp +").index",
    index,
    string.match(regexp).index );

  AddTestCase(
    "( " + string + " ).match(" + str_regexp +").input",
    string,
    string.match(regexp).input );

  for ( var matches = 0; matches < matches_array.length; matches++ ) {
    AddTestCase(
      "( " + string + " ).match(" + str_regexp +")[" + matches +"]",
      matches_array[matches],
      string.match(regexp)[matches] );
  }
}

function AddGlobalRegExpCases(
  regexp, str_regexp, string, length, matches_array ) {

  AddTestCase(
    "( " + string  + " ).match(" + str_regexp +").length",
    length,
    string.match(regexp).length );

  for ( var matches = 0; matches < matches_array.length; matches++ ) {
    AddTestCase(
      "( " + string + " ).match(" + str_regexp +")[" + matches +"]",
      matches_array[matches],
      string.match(regexp)[matches] );
  }
}
