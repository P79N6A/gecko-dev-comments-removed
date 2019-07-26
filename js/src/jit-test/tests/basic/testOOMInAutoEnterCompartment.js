

if (getBuildConfiguration()['asan'] && getBuildConfiguration()['debug']) quit(0);

function fatty() {
    try {
        fatty();
    } catch (e) {
        foo();
    }
}

if (!getBuildConfiguration()['root-analysis']) { 
    foo = evalcx("(function foo() { foo.bar() })");
    foo.bar = evalcx("(function bar() {})");

    fatty();
}
