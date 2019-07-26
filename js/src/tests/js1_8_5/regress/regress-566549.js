





try {
    evalcx('var p;', []);
} catch (exc) {}

try {
    evalcx('');
    Function("evalcx(\"var p\",[])")();
} catch (exc) {}

try {
    evalcx('var p;');
} catch (exc) {}

reportCompare(0, 0, "ok");
