































var SECTION = "ecma_2/String/split-003.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.split( regexp, [,limit] )";

startTest();







AddSplitCases( "hello", new RegExp, "new RegExp", ["h","e","l","l","o"] );

AddSplitCases( "hello", /l/, "/l/", ["he","","o"] );
AddLimitedSplitCases( "hello", /l/, "/l/", 0, [] );
AddLimitedSplitCases( "hello", /l/, "/l/", 1, ["he"] );
AddLimitedSplitCases( "hello", /l/, "/l/", 2, ["he",""] );
AddLimitedSplitCases( "hello", /l/, "/l/", 3, ["he","","o"] );
AddLimitedSplitCases( "hello", /l/, "/l/", 4, ["he","","o"] );
AddLimitedSplitCases( "hello", /l/, "/l/", void 0, ["he","","o"] );
AddLimitedSplitCases( "hello", /l/, "/l/", "hi", [] );
AddLimitedSplitCases( "hello", /l/, "/l/", undefined, ["he","","o"] );

AddSplitCases( "hello", new RegExp, "new RegExp", ["h","e","l","l","o"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", 0, [] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", 1, ["h"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", 2, ["h","e"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", 3, ["h","e","l"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", 4, ["h","e","l","l"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", void 0,  ["h","e","l","l","o"] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", "hi",  [] );
AddLimitedSplitCases( "hello", new RegExp, "new RegExp", undefined,  ["h","e","l","l","o"] );

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
  string, separator, str_sep, limit, split_array ) {

  

  AddTestCase(
    "( " + string  + " ).split(" + str_sep +", " + limit +
    " ).constructor == Array",
    true,
    string.split(separator, limit).constructor == Array );

  

  AddTestCase(
    "( " + string + " ).split(" + str_sep  +", " + limit + " ).length",
    split_array.length,
    string.split(separator, limit).length );

  

  var slimit = (split_array.length > string.split(separator).length )
    ? split_array.length : string.split(separator, limit).length;

  for ( var matches = 0; matches < slimit; matches++ ) {
    AddTestCase(
      "( " + string + " ).split(" + str_sep +", " + limit + " )[" + matches +"]",
      split_array[matches],
      string.split( separator, limit )[matches] );
  }
}
