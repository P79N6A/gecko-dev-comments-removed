





var x = {};
x.watch("p", function () { evalcx(''); });
x.p = 0;


watch("e", (function () { evalcx(''); }));
e = function () {};

reportCompare(0, 0, "ok");
