










if (String(undefined) !== "undefined") {
  $ERROR('#1: String(undefined) === "undefined". Actual: ' + (String(undefined)));
}


if (String(void 0) !== "undefined") {
  $ERROR('#2: String(void 0) === "undefined". Actual: ' + (String(void 0)));
}


if (String(eval("var x")) !== "undefined") {
  $ERROR('#3: String(eval("var x")) === "undefined" . Actual: ' + (String(eval("var x"))));
}

