










if (typeof "1" !== "string") {
  $ERROR('#1: typeof "1" === "string". Actual: ' + (typeof "1"));
}


if (typeof "NaN" !== "string") {
  $ERROR('#2: typeof "NaN" === "string". Actual: ' + (typeof "NaN"));
}


if (typeof "Infinity" !== "string") {
  $ERROR('#3: typeof "Infinity" === "string". Actual: ' + (typeof "Infinity"));
}


if (typeof "" !== "string") {
  $ERROR('#4: typeof "" === "string". Actual: ' + (typeof ""));
}


if (typeof "true" !== "string") {
  $ERROR('#5: typeof "true" === "string". Actual: ' + (typeof "true"));
}


if (typeof Date() !== "string") {
  $ERROR('#6: typeof Date() === "string". Actual: ' + (typeof Date()));
}

