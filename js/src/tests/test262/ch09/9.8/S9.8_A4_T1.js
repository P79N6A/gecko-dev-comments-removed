










var x1 = "abc";
if (String(x1) !== x1) {
  $ERROR('#1: String("abc") === "abc". Actual: ' + (String("abc")));
}


var x2 = "abc";
if (typeof String(x2) !== typeof x2) { 
  $ERROR('#2: typeof String("abc") === "string". Actual: ' + (typeof String("abc")));
}

