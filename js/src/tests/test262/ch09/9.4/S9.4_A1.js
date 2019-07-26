










if ("abc".charAt(Number.NaN) !== "a") {
  $ERROR('#1: "abc".charAt(Number.NaN) === "a". Actual: ' + ("abc".charAt(Number.NaN)));
}


if ("abc".charAt("x") !== "a") {
  $ERROR('#2: "abc".charAt("x") === "a". Actual: ' + ("abc".charAt("x")));
}

