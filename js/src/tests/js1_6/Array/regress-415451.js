




































var gTestfile = 'regress-415451.js';

var BUGNUMBER = 415451;
var summary = 'indexOf/lastIndexOf behavior';

var expected = "3,0,3,3,3,-1,-1";
results = [];
var a = [1,2,3,1];
for (var i=-1; i < a.length+2; i++)
  results.push(a.indexOf(1,i));
var actual = String(results);
reportCompare(expected, actual, "indexOf");

results = [];
var expected = "3,0,0,0,3,3,3";
for (var i=-1; i < a.length+2; i++)
  results.push(a.lastIndexOf(1,i));
var actual = String(results);
reportCompare(expected, actual, "lastIndexOf");

