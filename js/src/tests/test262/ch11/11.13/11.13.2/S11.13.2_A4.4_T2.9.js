










x = "1";
x += null;
if (x !== "1null") {
  $ERROR('#1: x = "1"; x += null; x === "1null". Actual: ' + (x));
}


x = null;
x += "1";
if (x !== "null1") {
  $ERROR('#2: x = null; x += "1"; x === "null1". Actual: ' + (x));
}


x = new String("1");
x += null;
if (x !== "1null") {
  $ERROR('#3: x = new String("1"); x += null; x === "1null". Actual: ' + (x));
}


x = null;
x += new String("1");
if (x !== "null1") {
  $ERROR('#4: x = null; x += new String("1"); x === "null1". Actual: ' + (x));
}

