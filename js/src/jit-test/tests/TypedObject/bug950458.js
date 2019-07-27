if (!this.hasOwnProperty("TypedObject"))
  quit();










var N = 100;
var T = TypedObject;
var Point = new T.StructType({x: T.uint32, y: T.uint32, z: T.uint32});
var PointArray = Point.array(N);
function foo(arr) {
  var sum = 0;
  for (var i = 0; i < N; i++) {
    sum += arr[i].x + arr[i].y + arr[i].z;
  }
}
foo(new PointArray());
