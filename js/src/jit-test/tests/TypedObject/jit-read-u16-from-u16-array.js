




if (!this.hasOwnProperty("TypedObject"))
  quit();

var Vec3u16Type = new TypedObject.ArrayType(TypedObject.uint16, 3);

function foo_u16() {
  for (var i = 0; i < 30000; i += 3) {
    var vec = new Vec3u16Type([i, i+1, i+2]);
    var sum = vec[0] + vec[1] + vec[2];
    assertEq(sum, 3*i + 3);
  }
}

foo_u16();
