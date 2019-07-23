





































var bug = 354541;
var summary = 'Regression to standard class constructors in case labels';
var actual = '';
var expect = '';


printBugNumber (bug);
printStatus (summary + ': top level');

String.prototype.trim = function() { print('hallo'); };

String.prototype.trim = function() { return 'hallo'; };

const S = String;
const Sp = String.prototype;

expect = 'hallo';
var expectStringInvariant = true
var actualStringInvariant;
var expectStringPrototypeInvariant = true;
var actualStringPrototypeInvariant;

if (typeof Script == 'undefined')
{
  print('Test skipped. Script not defined.');
  var actualStringInvariant = true;
  var actualStringPrototypeInvariant = true;
}
else
{
  var s = Script('var tmp = function(o) { switch(o) { case String: case 1: return ""; } }; actualStringInvariant = (String === S); actualStringPrototypeInvariant = (String.prototype === Sp); actual = "".trim();');
  try
  {
    s();
  }
  catch(ex)
  {
    actual = ex + '';
  }
}
  
reportCompare(expect, actual, 'trim() returned');
reportCompare(expectStringInvariant, actualStringInvariant, 
              'String invariant');
reportCompare(expectStringPrototypeInvariant, 
              actualStringPrototypeInvariant,
              'String.prototype invariant');

