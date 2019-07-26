











if ("abc".charAt(0.0) !== "a") {
  $ERROR('#1: "abc".charAt(0.0) === "a". Actual: ' + ("abc".charAt(0.0)));
}


if ("abc".charAt(-0.0) !== "a") {
  $ERROR('#2: "abc".charAt(-0.0) === "a". Actual: ' + ("abc".charAt(-0.0)));
}

