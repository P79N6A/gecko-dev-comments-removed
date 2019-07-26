

















var T = TypedObject;

function check(v) {
    return T.storage(v);
}

function test() {
    var v = new Object;         
    for ( var i=0 ; i < 1000 ; i++ )
        try { check(v); } catch (e) {}
    try { return check(v); } catch (e) {}
}

test();
