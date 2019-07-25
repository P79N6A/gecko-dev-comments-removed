



var expect = 42;
var actual = (function({
    x: [w]
},
x) {
    with({}) {return w;}
})({x:[42]});

reportCompare(expect, actual, "ok");
