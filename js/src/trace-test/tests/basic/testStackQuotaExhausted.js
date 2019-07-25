
const numFatArgs = Math.pow(2,19) - 1024;

const traceDepth = 490;

var trace = true;

function maybeTrace(x) {
    if (!trace)
        eval("");
    if (x <= 0)
        return 0;
    return maybeTrace(x-1);
}

function fatStack() {
    return maybeTrace(traceDepth);
}

function assertRightFailure(e) {
    assertEq(e.toString() == "InternalError: script stack space quota is exhausted" ||
             e.toString() == "InternalError: too much recursion",
	     true);
}



exception = false;
try {
    fatStack.apply(null, new Array(numFatArgs));
} catch (e) {
    assertRightFailure(e);
    exception = true;
}
assertEq(exception, true);


checkStats({traceCompleted:0});


trace = false;
var exception = false;
try {
    fatStack.apply(null, new Array(numFatArgs));
} catch (e) {
    assertRightFailure(e);
    exception = true;
}
assertEq(exception, true);
