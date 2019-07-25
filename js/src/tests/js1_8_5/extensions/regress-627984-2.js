



var obj = {};
var x;
obj.watch("m", function (id, oldval, newval) {
        x = this.m;
        return newval;
    });
delete obj.m;
obj.m = function () { return this.method; };
obj.m = 2;

reportCompare(0, 0, 'ok');
