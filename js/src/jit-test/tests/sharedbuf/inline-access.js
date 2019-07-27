















if (!this.SharedInt32Array)
    quit();

function f(ta) {
    return (ta[2] = ta[0] + ta[1] + ta.length);
}

var v = new SharedInt32Array(1024);
var sum = 0;
var iter = 1000;
for ( var i=0 ; i < iter ; i++ )
    sum += f(v);
assertEq(sum, v.length * iter);
