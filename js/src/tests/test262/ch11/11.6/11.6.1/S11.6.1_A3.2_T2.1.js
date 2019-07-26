










if ("1" + 1 !== "11") {
  $ERROR('#1: "1" + 1 === "11". Actual: ' + ("1" + 1));
}


if (1 + "1" !== "11") {
  $ERROR('#2: 1 + "1" === "11". Actual: ' + (1 + "1"));
}


if (new String("1") + 1 !== "11") {
  $ERROR('#3: new String("1") + 1 === "11". Actual: ' + (new String("1") + 1));
}


if (1 + new String("1") !== "11") {
  $ERROR('#4: 1 + new String("1") === "11". Actual: ' + (1 + new String("1")));
}


if ("1" + new Number(1) !== "11") {
  $ERROR('#5: "1" + new Number(1) === "11". Actual: ' + ("1" + new Number(1)));
}


if (new Number(1) + "1" !== "11") {
  $ERROR('#6: new Number(1) + "1" === "11". Actual: ' + (new Number(1) + "1"));
}


if (new String("1") + new Number(1) !== "11") {
  $ERROR('#7: new String("1") + new Number(1) === "11". Actual: ' + (new String("1") + new Number(1)));
}


if (new Number(1) + new String("1") !== "11") {
  $ERROR('#8: new Number(1) + new String("1") === "11". Actual: ' + (new Number(1) + new String("1")));
}


if ("x" + 1 !=="x1") {
  $ERROR('#9: "x" + 1 === "x1". Actual: ' + ("x" + 1));
}


if (1 + "x" !== "1x") {
  $ERROR('#10: 1 + "x" === "1x". Actual: ' + (1 + "x"));
}

