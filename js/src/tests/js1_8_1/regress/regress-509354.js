






var actual = "" + function ([arguments]) {return arguments;};
compareSource('function ([arguments]) {return arguments;}', actual, "part 1");


var f = function ([arguments]) {return arguments + 1;};
reportCompare(3.25, f([2.25]), "part 2");


actual = "no exception";
try {
    eval('(function ([arguments, arguments]) {return arguments();})');
} catch (exc) {
    actual = exc.name;
}
reportCompare("SyntaxError", actual, "part 3");


actual = "no exception";
try {
    eval('(function ([a, b, arguments, d], [e, f, arguments]) {return arguments();})');
} catch (exc) {
    actual = exc.name;
}
reportCompare("SyntaxError", actual, "part 4");


try {
    eval('print(function([arguments,arguments,arguments,arguments,arguments,' +
         'arguments,arguments,arguments,arguments,arguments,arguments,' +
         'arguments,arguments,arguments,arguments,arguments]){})');
} catch (exc) {
}
reportCompare("no crash", "no crash", "part 5");

