





function f(s) {
  eval(s);
  return function(a) {
    var d;
    let (c = 3) {
      d = function() { a; }; 
      with({}) {}; 
      return b; 
    }
  };
}

var b = 1;
var g1 = f("");
var g2 = f("var b = 2;");


g1(0);





assertEq(g2(0), 2);

reportCompare(true, true);
