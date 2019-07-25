































var SECTION = "ecma_2/String/split-001.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.split( regexp, [,limit] )";

startTest();





AddSplitCases( "splitme", "", "''", ["s", "p", "l", "i", "t", "m", "e"] );
AddSplitCases( "splitme", new RegExp(), "new RegExp()", ["s", "p", "l", "i", "t", "m", "e"] );













test();

function AddSplitCases( string, separator, str_sep, split_array ) {

  
  AddTestCase(
    "( " + string  + " ).split(" + str_sep +").constructor == Array",
    true,
    string.split(separator).constructor == Array );

  
  AddTestCase(
    "( " + string  + " ).split(" + str_sep +").length",
    split_array.length,
    string.split(separator).length );

  
  var limit = (split_array.length > string.split(separator).length )
    ? split_array.length : string.split(separator).length;

  for ( var matches = 0; matches < split_array.length; matches++ ) {
    AddTestCase(
      "( " + string + " ).split(" + str_sep +")[" + matches +"]",
      split_array[matches],
      string.split( separator )[matches] );
  }
}

function AddLimitedSplitCases(
  string, separator, str_sep, limit, str_limit, split_array ) {

  

  AddTestCase(
    "( " + string  + " ).split(" + str_sep +", " + str_limit +
    " ).constructor == Array",
    true,
    string.split(separator, limit).constructor == Array );

  

  AddTestCase(
    "( " + string + " ).split(" + str_sep  +", " + str_limit + " ).length",
    length,
    string.split(separator).length );

  

  for ( var matches = 0; matches < split_array.length; matches++ ) {
    AddTestCase(
      "( " + string + " ).split(" + str_sep +", " + str_limit + " )[" + matches +"]",
      split_array[matches],
      string.split( separator )[matches] );
  }
}
