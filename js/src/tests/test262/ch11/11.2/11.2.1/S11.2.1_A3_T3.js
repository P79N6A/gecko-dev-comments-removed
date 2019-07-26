










if ("abc123".charAt(5) !== "3") {
  $ERROR('#1: "abc123".charAt(5) === "3". Actual: ' + ("abc123".charAt(5)));
}


if ("abc123"["charAt"](0) !== "a") {
  $ERROR('#2: "abc123"["charAt"](0) === "a". Actual: ' + ("abc123"["charAt"](0)));
}


if ("abc123".length !== 6) {
  $ERROR('#3: "abc123".length === 6. Actual: ' + ("abc123".length));
}


if ("abc123"["length"] !== 6) {
  $ERROR('#4: "abc123"["length"] === 6. Actual: ' + ("abc123"["length"]));
}


if (new String("abc123").length !== 6) {
  $ERROR('#5: new String("abc123").length === 6. Actual: ' + (new String("abc123").length));
}


if (new String("abc123")["charAt"](2) !== "c") {
  $ERROR('#6: new String("abc123")["charAt"](2) === "c". Actual: ' + (new String("abc123")["charAt"](2)));
}  

