
function f(x) {
    return x + 1;
}

setJitCompilerOption("ion.usecount.trigger", 2);
setJitCompilerOption("baseline.usecount.trigger", 0);

assertEq(f(1), 2);     
assertEq(f(0.5), 1.5); 
                       


function normal() {
    setJitCompilerOption("ion.usecount.trigger", 8);
    setJitCompilerOption("baseline.usecount.trigger", 5);
}

function eager() {
    setJitCompilerOption("ion.usecount.trigger", 0);
}

function h(x) {
    return x + 1;
}

function g(x) {
    normal();
    return h(x) + 1;
}

normal();
for (var i = 0; i < 10; i++) {
    eager();
    assertEq(g(i), i + 2);
}



try {
    setJitCompilerOption("not.an.option", 51);
    assertEq(false, true);
} catch (x) { }

try {
    var ion = { usecount: { trigger: null } };
    setJitCompilerOption(ion.usecount.trigger, 42);
    assertEq(false, true);
} catch (x) { }

try {
    setJitCompilerOption("ion.usecount.trigger", "32");
    assertEq(false, true);
} catch (x) { }
