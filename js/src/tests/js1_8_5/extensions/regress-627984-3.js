




var obj = {};
obj.watch("m", function (id, oldval, newval) {
        return 'ok';
    });
delete obj.m;
obj.m = function () { return this.x; };
assertEq(obj.m, 'ok');

reportCompare(0, 0, 'ok');
