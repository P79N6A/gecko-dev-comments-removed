




if (!this.hasOwnProperty("TypedObject"))
  quit();



var A = TypedObject.uint8.array();
var a = new A(0);
a.forEach(function(val, i) {});



var AA = TypedObject.uint8.array(2147483647).array();
var aa = new AA(0);
