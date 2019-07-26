










x = "1";
x += undefined;
if (x !== "1undefined") {
  $ERROR('#1: x = "1"; x += undefined; x === "1undefined". Actual: ' + (x));
}


x = undefined;
x += "1";
if (x !== "undefined1") {
  $ERROR('#2: x = undefined; x += "1"; x === "undefined1". Actual: ' + (x));
}


x = new String("1");
x += undefined;
if (x !== "1undefined") {
  $ERROR('#3: x = new String("1"); x += undefined; x === "1undefined". Actual: ' + (x));
}


x = undefined;
x += new String("1");
if (x !== "undefined1") {
  $ERROR('#4: x = undefined; x += new String("1"); x === "undefined1". Actual: ' + (x));
}

