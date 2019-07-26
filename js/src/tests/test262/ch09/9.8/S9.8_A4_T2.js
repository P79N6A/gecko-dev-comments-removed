










var x1 = "abc";
if (x1 + "" !== x1) {
  $ERROR('#1: "abc" + "" === "abc". Actual: ' + ("abc" + ""));
}


var x2 = "abc";
if (typeof x2 + "" !== typeof x2) { 
  $ERROR('#2: typeof "abc" + "" === "string". Actual: ' + (typeof "abc" + ""));
}

