










var object = {};
object["true"] = 1;
if (true in object !== "true" in object) {  
  $ERROR('#1: "var object = {}; object["true"] = 1; true in object === "true" in object');  
}


var object = {};
object.Infinity = 1;
if (Infinity in object !== "Infinity" in object) {  
  $ERROR('#2: "var object = {}; object.Infinity = 1; Infinity in object === "Infinity" in object');  
}


var object = {};
object.undefined = 1;
if (undefined in object !== "undefined" in object) {  
  $ERROR('#4: "var object = {}; object.undefined = 1; undefined in object === "undefined" in object');  
}


var object = {};
object["null"] = 1;
if (null in object !== "null" in object) {  
  $ERROR('#5: "var object = {}; object["null"] = 1; null in object === "null" in object');  
}

