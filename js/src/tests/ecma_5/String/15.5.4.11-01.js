




var BUGNUMBER = 587366;
var summary = "String.prototype.replace with non-regexp searchValue";

print(BUGNUMBER + ": " + summary);










/(a|(b)|c)+/.exec('abcabc');
var before = {
    "source" : RegExp.source,
    "$`": RegExp.leftContext,
    "$'": RegExp.rightContext,
    "$&": RegExp.lastMatch,
    "$1": RegExp.$1,
    "$2": RegExp.$2
};

var text = 'I once was lost but now am found.';
var searchValue = 'found';
var replaceValue;


replaceValue = function(matchStr, matchStart, textStr) {
    assertEq(matchStr, searchValue);
    assertEq(matchStart, 27);
    assertEq(textStr, text);
    return 'not watching that show anymore';
}
var result = text.replace(searchValue, replaceValue);
assertEq(result, 'I once was lost but now am not watching that show anymore.');


replaceValue = "...wait, where was I again? And where is all my $$$$$$? Oh right, $`$&$'" +
               " But with no $$$$$$"; 
result = text.replace(searchValue, replaceValue);
assertEq(result, 'I once was lost but now am ...wait, where was I again?' +
                 ' And where is all my $$$? Oh right, I once was lost but now am found.' +
                 ' But with no $$$.');


replaceValue = "$1$&$2$'$3";
result = text.replace(searchValue, replaceValue);
assertEq(result, 'I once was lost but now am $1found$2.$3.');


for (var ident in before)
    assertEq(RegExp[ident], before[ident]);



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
