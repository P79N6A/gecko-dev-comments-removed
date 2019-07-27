

















if (!this.TypedObject)
  quit();

var T = TypedObject;

function check(v) {
    return T.storage(v);
}

function test() {
    var AT = new T.ArrayType(T.Any,10);
    var v = new AT();
    for ( var i=0 ; i < 1000 ; i++ )
        check(v);
    return check(v);
}

test();
