



















var T = TypedObject;
var ST1 = new T.StructType({x:T.int32});
var ST2 = new T.StructType({x:T.float64});

function check(v) {
    return v.toSource();
}

function test() {
  var a = [ ST1, ST2 ];
    for ( var i=0 ; i < 1000 ; i++ )
	check(a[i%2]);
    return check(a[0]);
}

print(test());


