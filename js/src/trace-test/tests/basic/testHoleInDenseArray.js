var s;

function f(i) {
    if (i > 4) 
        assertEq(s, undefined);
    else
        assertEq(s, false);
    return 1;
}


var arr = [ false, false, false, false, false, , , , , , true ];

for (var i = 0; i < 10; ++i) {
    (s = arr[i]) + f(i);
}

checkStats({ traceTriggered: 2, sideExitIntoInterpreter: 2 })
