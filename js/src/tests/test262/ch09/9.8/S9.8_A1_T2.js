










if (undefined + "" !== "undefined") {
  $ERROR('#1: undefined + "" === "undefined". Actual: ' + (undefined + ""));
}


if (void 0 + "" !== "undefined") {
  $ERROR('#2: void 0 + "" === "undefined". Actual: ' + (void 0 + ""));
}


if (eval("var x") + "" !== "undefined") {
  $ERROR('#3: eval("var x") + "" === "undefined". Actual: ' + (eval("var x") + ""));
}

