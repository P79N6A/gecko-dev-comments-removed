

if (!this.hasOwnProperty("TypedObject"))
  throw new TypeError();




var AA = TypedObject.uint8.array(2147483647).array();
var aa = new AA(-1);

