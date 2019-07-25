



var x;
for (let i = 0; i < 2; i++)
    x = {m: function () {}, n: function () { i++; }};
x.m;

reportCompare(0, 0, 'ok');
