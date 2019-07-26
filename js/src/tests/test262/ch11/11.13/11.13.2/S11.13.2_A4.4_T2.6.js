










x = "1";
x += 1;
if (x !== "11") {
  $ERROR('#1: x = "1"; x += 1; x === "11". Actual: ' + (x));
}


x = 1;
x += "1";
if (x !== "11") {
  $ERROR('#2: x = 1; x += "1"; x === "11". Actual: ' + (x));
}


x = new String("1");
x += 1;
if (x !== "11") {
  $ERROR('#3: x = new String("1"); x += 1; x === "11". Actual: ' + (x));
}


x = 1;
x += new String("1");
if (x !== "11") {
  $ERROR('#4: x = 1; x += new String("1"); x === "11". Actual: ' + (x));
}


x = "1";
x += new Number(1);
if (x !== "11") {
  $ERROR('#5: x = "1"; x += new Number(1); x === "11". Actual: ' + (x));
}


x = new Number(1);
x += "1";
if (x !== "11") {
  $ERROR('#6: x = new Number(1); x += "1"; x === "11". Actual: ' + (x));
}


x = new String("1");
x += new Number(1);
if (x !== "11") {
  $ERROR('#7: x = new String("1"); x += new Number(1); x === "11". Actual: ' + (x));
}


x = new Number(1);
x += new String("1");
if (x !== "11") {
  $ERROR('#8: x = new Number(1); x += new String("1"); x === "11". Actual: ' + (x));
}


if ("x" + 1 !=="x1") {
  $ERROR('#9: x = "x"; x += 1; x === "x1". Actual: ' + (x));
}


x = 1;
x += "x";
if (x !== "1x") {
  $ERROR('#10: x = 1; x += "x"; x === "1x". Actual: ' + (x));
}

