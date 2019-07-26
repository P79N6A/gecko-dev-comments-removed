










if (typeof true !== "boolean") {
  $ERROR('#1: typeof true === "boolean". Actual: ' + (typeof true));
}


if (typeof false !== "boolean") {
  $ERROR('#2: typeof false === "boolean". Actual: ' + (typeof false));
}


if (typeof !-1 !== "boolean") {
  $ERROR('#3: typeof !-1 === "boolean". Actual: ' + (typeof !-1));
}

