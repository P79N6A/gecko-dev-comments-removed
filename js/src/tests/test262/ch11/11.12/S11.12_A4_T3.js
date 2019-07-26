










if (("1" ? "" : "1") !== "") {
  $ERROR('#1: ("1" ? "" : "1") === ""');
}


var y = new String("1");
if (("1" ? y : "") !== y) {
  $ERROR('#2: (var y = new String("1"); ("1" ? y : "") === y');
}


var y = new String("y");
if ((y ? y : "1") !== y) {
  $ERROR('#3: (var y = new String("y"); (y ? y : "1") === y');
}

