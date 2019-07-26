



var obj = {
    f: function () {
        return (this for (x of [0]));
    }
};

assertEq(obj.f().next(), obj);
