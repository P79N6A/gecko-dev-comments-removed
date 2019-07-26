










var result = function f(o) {o.x = 1; return o;};
(new Object()).x;
if (typeof result !== "function") {
  $ERROR('#1: Check Function Expression for automatic semicolon insertion');
}

