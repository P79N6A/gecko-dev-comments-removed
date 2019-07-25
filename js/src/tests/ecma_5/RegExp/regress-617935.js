







expectExitCode(0);
expectExitCode(5);


var foo = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";


for (i = 0; i < 10; ++i) {
    foo += foo;
}


foo += "a";

var bar = "bbbbbbbbbbbbbbbb";


for (i = 0; i < 9; ++i) {
    bar += bar;
}






try {
    foo.replace(/[a]/g, bar);
} catch (e) {
    reportCompare(e instanceof InternalError, true, "Internal error due to overallocation is ok.");
}
reportCompare(true, true, "No crash occurred.");

print("Tests complete");
