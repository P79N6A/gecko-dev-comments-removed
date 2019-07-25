
function foo1() {
    return foo1.caller.p;
}

function bar1() {
    foo1(0, 1);
}

bar1();
bar1();



function foo2() {
    [][0]; 
    dumpStack();
}

function bar2() {
    foo2(0, 1);
}

bar2();
bar2();
