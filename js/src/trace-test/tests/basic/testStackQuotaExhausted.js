
const numFatArgs = Math.pow(2,19) - 1024;

const traceDepth = 490;

var trace = true;

function doEval() {
    eval("");
}

function maybeTrace(x) {
    if (!trace)
        doEval();
    if (x <= 0)
        return 0;
    return maybeTrace(x-1);
}

function fatStack() {
    return maybeTrace(traceDepth);
}



exception = false;
try {
    fatStack.apply(null, new Array(numFatArgs));
} catch (e) {
    assertEq(e.toString(), "InternalError: script stack space quota is exhausted");
    exception = true;
}
assertEq(exception, true);
checkStats({traceCompleted:1});


trace = false;
var exception = false;
try {
    fatStack.apply(null, new Array(numFatArgs));
} catch (e) {
    assertEq(e.toString(), "InternalError: script stack space quota is exhausted");
    exception = true;
}
assertEq(exception, true);
