





function f(x) { return 1 + "" + (x + 1); }
reportCompare("12", f(1), "");
var g = eval(String(f));
reportCompare("12", f(1), "");
