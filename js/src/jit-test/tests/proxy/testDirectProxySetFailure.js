

load(libdir + "asserts.js");

var obj = {x: 1};
var p = new Proxy(obj, {
    set(target, key, value, receiver) { return false; }
});


assertEq(p.x = 2, 2);
assertEq(obj.x, 1);


assertThrowsInstanceOf(() => { "use strict"; p.x = 2; }, TypeError);
assertEq(obj.x, 1);


assertThrowsInstanceOf(() => { "use strict"; p.x = 1; }, TypeError);
assertEq(obj.x, 1);


assertThrowsInstanceOf(() => { "use strict"; p.z = 1; }, TypeError);
assertEq("z" in obj, false);


var arr = ["not", "already", "in", "order"];
var p2 = new Proxy(arr, {
    get(target, key, receiver) {
        if (key === "length" && new Proxy([1], {}).length === 1) {
            throw new Error("Congratulations! You have fixed bug 895223 at last! " +
                            "Please delete this whole get() method which is a " +
                            "workaround for that bug.");
        }
        return target[key];
    },
    set(target, key, value, receiver) { return false; }
});
assertThrowsInstanceOf(() => p2.sort(), TypeError);
assertDeepEq(arr, ["not", "already", "in", "order"]);
