




var actual = 'No error';
try {
    eval("var let = true");
} catch (exc) {
    actual = exc + '';
}

reportCompare('SyntaxError: let is a reserved identifier', actual, 'ok'); 

