

var set = Set("abcd");
var iter = set.iterator();
var log = "";
for (let x of iter) {
    log += x;
    if (x === "b")
        set.delete("c");
}
assertEq(log, "abd");
