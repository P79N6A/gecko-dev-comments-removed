












if (String(1) !== "1") {
  $ERROR('#1: String(1) === "1". Actual: ' + (String(1)));
}


if (String(10) !== "10") {
  $ERROR('#2: String(10) === "10". Actual: ' + (String(10)));
}


if (String(100) !== "100") {
  $ERROR('#3: String(100) === "100". Actual: ' + (String(100)));
}


if (String(100000000000000000000) !== "100000000000000000000") {
  $ERROR('#4: String(100000000000000000000) === "100000000000000000000". Actual: ' + (String(100000000000000000000)));
}


if (String(1e20) !== "100000000000000000000") {
  $ERROR('#5: String(1e20) === "100000000000000000000". Actual: ' + (String(1e20)));
}


if (String(12345) !== "12345") {
  $ERROR('#6: String(12345) === "12345". Actual: ' + (String(12345)));
}


if (String(12345000) !== "12345000") {
  $ERROR('#7: String(12345000) === "12345000". Actual: ' + (String(12345000)));
}


if (String(-1) !== "-1") {
  $ERROR('#8: String(-1) === "-1". Actual: ' + (String(-1)));
}


if (String(-10) !== "-10") {
  $ERROR('#9: String(-10) === "-10". Actual: ' + (String(-10)));
}


if (String(-100) !== "-100") {
  $ERROR('#3: String(-100) === "-100". Actual: ' + (String(-100)));
}


if (String(-100000000000000000000) !== "-100000000000000000000") {
  $ERROR('#10: String(-100000000000000000000) === "-100000000000000000000". Actual: ' + (String(-100000000000000000000)));
}


if (String(-1e20) !== "-100000000000000000000") {
  $ERROR('#11: String(-1e20) === "-100000000000000000000". Actual: ' + (String(-1e20)));
}


if (String(-12345) !== "-12345") {
  $ERROR('#12: String(-12345) === "-12345". Actual: ' + (String(-12345)));
}


if (String(-12345000) !== "-12345000") {
  $ERROR('#13: String(-12345000) === "-12345000". Actual: ' + (String(-12345000)));
}


if (String(1E20) !== "100000000000000000000") {
  $ERROR('#14: String(1E20) === "100000000000000000000". Actual: ' + (String(1E20)));
}


if (String(-1E20) !== "-100000000000000000000") {
  $ERROR('#15: String(-1E20) === "-100000000000000000000". Actual: ' + (String(-1E20)));
}


