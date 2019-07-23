




































var gTestfile = 'destructuring-scope.js';

var BUGNUMBER = 'none';
var summary = 'Test destructuring assignments for differing scopes';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f() {
  var x = 3;
  if (x > 0) {
    let {a:x} = {a:7};
    if (x != 7) 
      throw "fail";
  }
  if (x != 3) 
    throw "fail";
}

function g() {
  for (var [a,b] in {x:7}) {
    if (a != "x" || b != 7) 
      throw "fail";
  }

  {
    for (let [a,b] in {y:8}) {
      if (a != "y" || b != 8) 
        throw "fail";
    }
  }

  if (a != "x" || b != 7) 
    throw "fail";
}

f();
g();

if (typeof a != "undefined" || typeof b != "undefined" || typeof x != "undefined")
  throw "fail";

function h() {
  for ([a,b] in {z:9}) {
    if (a != "z" || b != 9) 
      throw "fail";
  }
}

h();

if (a != "z" || b != 9) 
  throw "fail";


reportCompare(expect, actual, summary);

