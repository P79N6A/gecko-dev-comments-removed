


var gTestfile = 'parseInt-01.js';

var BUGNUMBER = 577536;
var summary = "ES5 15.1.2.2 parseInt(string, radix)";

print(BUGNUMBER + ": " + summary);





var str, radix;
var upvar;



assertEq(parseInt({ toString: function() { return "17" } }, 10), 17);

upvar = 0;
str = { get toString() { upvar++; return function() { upvar++; return "12345"; } } };
assertEq(parseInt(str, 10), 12345);
assertEq(upvar, 2);








var ws =
  ["\t", "\v", "\f", " ", "\xA0", "\uFEFF",
     "\u2004", "\u3000", 
   "\r", "\n", "\u2028", "\u2029"];

str = "8675309";
for (var i = 0, sz = ws.length; i < sz; i++)
{
  assertEq(parseInt(ws[i] + str, 10), 8675309);
  for (var j = 0, sz = ws.length; j < sz; j++)
  {
    assertEq(parseInt(ws[i] + ws[j] + str, 10), 8675309,
             ws[i].charCodeAt(0).toString(16) + ", " +
             ws[j].charCodeAt(0).toString(16));
  }
}







str = "5552368";
assertEq(parseInt("-" + str, 10), -parseInt(str, 10));
assertEq(parseInt(" -" + str, 10), -parseInt(str, 10));
assertEq(parseInt("-", 10), NaN);
assertEq(parseInt("", 10), NaN);
assertEq(parseInt("-0", 10), -0);






assertEq(parseInt("+12345", 10), 12345);
assertEq(parseInt(" +12345", 10), 12345);






upvar = "";
str =
  { toString: function() { if (!upvar) upvar = "string"; return "42"; } };
radix =
  { toString: function() { if (!upvar) upvar = "radix"; return "10"; } };

assertEq(parseInt(str, radix), 42);
assertEq(upvar, "string");

assertEq(parseInt("123", null), 123);
assertEq(parseInt("123", undefined), 123);
assertEq(parseInt("123", NaN), 123);
assertEq(parseInt("123", -0), 123);
assertEq(parseInt("10", 72057594037927950), 16);
assertEq(parseInt("10", -4294967292), 4);
assertEq(parseInt("0x10", 1e308), 16);
assertEq(parseInt("10", 1e308), 10);
assertEq(parseInt("10", { valueOf: function() { return 16; } }), 16);














var vs = ["1", "51", "917", "2343", "99963"];
for (var i = 0, sz = vs.length; i < sz; i++)
  assertEq(parseInt(vs[i], 0), parseInt(vs[i], 10), "bad " + vs[i]);

assertEq(parseInt("0x10"), 16);
assertEq(parseInt("0x10", 0), 16);
assertEq(parseInt("0x10", 16), 16);
assertEq(parseInt("0x10", 8), 0);
assertEq(parseInt("-0x10", 16), -16);

assertEq(parseInt("5", 1), NaN);
assertEq(parseInt("5", 37), NaN);
assertEq(parseInt("5", { valueOf: function() { return -1; } }), NaN);








assertEq(parseInt(""), NaN);
assertEq(parseInt("ohai"), NaN);
assertEq(parseInt("0xohai"), NaN);
assertEq(parseInt("-ohai"), NaN);
assertEq(parseInt("+ohai"), NaN);
assertEq(parseInt(" ohai"), NaN);

assertEq(parseInt("0xaohai"), 10);
assertEq(parseInt("hohai", 18), 17);














assertEq(parseInt("ohai", 36), 1142154);




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
