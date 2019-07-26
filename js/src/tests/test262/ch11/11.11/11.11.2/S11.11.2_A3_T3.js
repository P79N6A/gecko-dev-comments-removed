










if (("" || "1") !== "1") {
  $ERROR('#1: ("" || "1") === "1"');
}


var y = new String("1");
if (("" || y) !== y) {
  $ERROR('#2: (var y = new String("1"); "" || y) === y');
}

