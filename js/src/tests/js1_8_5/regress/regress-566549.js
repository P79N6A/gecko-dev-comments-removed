





try {
    evalcx('var p;', []);
} catch (exc) {}

try {
    evalcx('');
    Function("evalcx(\"var p\",[])")();
} catch (exc) {}

try {
    evalcx('var p;', <x/>);
} catch (exc) {}

try {
    evalcx('var p;', <x><p/></x>);
} catch (exc) {}

reportCompare(0, 0, "ok");
