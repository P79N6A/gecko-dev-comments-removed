










if (typeof null !== "object") {
  $ERROR('#1: typeof null === "object". Actual: ' + (typeof null));
}


if (typeof RegExp("0").exec("1") !== "object") {
  $ERROR('#2: typeof RegExp("0").exec("1") === "object". Actual: ' + (typeof RegExp("0").exec("1")));
}

