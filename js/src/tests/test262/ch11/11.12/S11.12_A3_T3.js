










if (("" ? "" : "1") !== "1") {
  $ERROR('#1: ("" ? "" : "1") === "1"');
}


var z = new String("1");
if (("" ? "1" : z) !== z) {
  $ERROR('#2: (var y = new String("1"); ("" ? "1" : z) === z');
}

