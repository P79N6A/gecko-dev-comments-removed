


















if (!this.TypedObject)
  quit();

var T = TypedObject;

function check(v) {
    return T.storage(v);
}

function test() {
    var AT = new T.ArrayType(T.int32,10);
    var v = new Object;         
    var w = new AT();           
    var a = [v,w];
    for ( var i=0 ; i < 1000 ; i++ )
        try { check(a[i%2]); } catch (e) {}
    try { return check(a[1]); } catch (e) {}
}

test();
