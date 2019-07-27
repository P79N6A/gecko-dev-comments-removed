

if (!this.SharedUint16Array)
    quit();

var thrown = false;
try {
    new SharedUint16Array(2147483647); 
}
catch (e) {
    thrown = true;
}
assertEq(thrown, true);

var thrown = false;
try {
    new SharedUint16Array(0xdeadbeef); 
}
catch (e) {
    thrown = true;
}
assertEq(thrown, true);
