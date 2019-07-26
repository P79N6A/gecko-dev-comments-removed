


var obj = {
    f: function () {
        assertEq(this, obj);
        return (this for (x of [0]));
    }
};

assertEq(obj.f().next(), obj);
