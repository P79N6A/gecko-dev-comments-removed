










if (String(Infinity) !== "Infinity") {
  $ERROR('#1: String(Infinity) === "Infinity". Actual: ' + (String(Infinity)));
}


if (String(Number.POSITIVE_INFINITY) !== "Infinity") {
  $ERROR('#2: String(Number.POSITIVE_INFINITY) === "Infinity". Actual: ' + (String(Number.POSITIVE_INFINITY)));
}


if (String(-Infinity) !== "-Infinity") {
  $ERROR('#3: String(-Infinity) === "-Infinity". Actual: ' + (String(-Infinity)));
}


if (String(Number.NEGATIVE_INFINITY) !== "-Infinity") {
  $ERROR('#4: String(Number.NEGATIVE_INFINITY) === "-Infinity". Actual: ' + (String(Number.NEGATIVE_INFINITY)));
}

