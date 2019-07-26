




















if (!this.TypedObject) {
    print("No TypedObject, skipping");
    quit();
}

var T = TypedObject;
var ST1 = new T.StructType({x:T.int32});
var ST2 = new T.StructType({f:T.float64});
var v1 = new ST1({x:10});
var v2 = new ST2({f:3.14159});

function check(v) {
  return T.objectType(v);
}

function test() {
  var a = [ v1, v2 ];
  for ( var i=0 ; i < 1000 ; i++ )
    assertEq(check(a[i%2]), ((i%2)==0 ? ST1 : ST2));
  return check(a[i%2]);
}

print("Done");
