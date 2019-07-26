










if (typeof 1 !== "number") {
  $ERROR('#1: typeof 1 === "number". Actual: ' + (typeof 1));
}


if (typeof Number.NaN !== "number") {
  $ERROR('#2: typeof NaN === "number". Actual: ' + (typeof NaN));
}


if (typeof Number.POSITIVE_INFINITY !== "number") {
  $ERROR('#3: typeof Infinity === "number". Actual: ' + (typeof Infinity));
}


if (typeof Number.NEGATIVE_INFINITY !== "number") {
  $ERROR('#4: typeof -Infinity === "number". Actual: ' + (typeof -Infinity));
}


if (typeof Math.PI !== "number") {
  $ERROR('#5: typeof Math.PI === "number". Actual: ' + (typeof Math.PI));
}

