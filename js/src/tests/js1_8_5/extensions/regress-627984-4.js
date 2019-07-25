



var obj = {};
obj.watch("m", function (id, oldval, newval) {
        delete obj.m;
        obj.m = function () {};
        dumpObject(obj);
        return newval;
    });
delete obj.m;
obj.m = 1;
assertEq(obj.m, 1);

reportCompare(0, 0, 'ok');
