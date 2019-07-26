

load(libdir + "asserts.js");

var mistakes = [
    "function f(...rest) { return x => arguments; }",
    "function f(...rest) { return (x=arguments) => 0; }"
];

for (var s of mistakes)
    assertThrowsInstanceOf(function () { eval(s); }, SyntaxError);
