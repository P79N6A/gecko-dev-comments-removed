










if ("1" + "1" !== "11") {
  $ERROR('#1: "1" + "1" === "11". Actual: ' + ("1" + "1"));
}


if (new String("1") + "1" !== "11") {
  $ERROR('#2: new String("1") + "1" === "11". Actual: ' + (new String("1") + "1"));
}


if ("1" + new String("1") !== "11") {
  $ERROR('#3: "1" + new String("1") === "11". Actual: ' + ("1" + new String("1")));
}


if (new String("1") + new String("1") !== "11") {
  $ERROR('#4: new String("1") + new String("1") === "11". Actual: ' + (new String("1") + new String("1")));
}


if ("x" + "1" !=="x1") {
  $ERROR('#5: "x" + "1" === "x1". Actual: ' + ("x" + "1"));
}


if ("1" + "x" !== "1x") {
  $ERROR('#6: "1" + "x" === "1x". Actual: ' + ("1" + "x"));
}

