





try {
    for (var [e] = /x/ in d) {
        (function () {});
    }
} catch (e) {}
try {
    let(x = Object.freeze(this, /x/))
    e = #0= * .toString
    function y() {}
} catch (e) {}

reportCompare(0, 0, "don't crash");
