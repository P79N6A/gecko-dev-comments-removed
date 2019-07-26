



















var T = TypedObject;
var ST = new T.StructType({x:T.int32});

function check(v) {
    return v.toSource();
}

function test() {
    var fake = { toSource: ST.toSource };
    for ( var i=0 ; i < 1000 ; i++ )
	try { check(fake); } catch (e) {}
    try { return check(fake); } catch (e) { return "Thrown" }
}

print(test());

