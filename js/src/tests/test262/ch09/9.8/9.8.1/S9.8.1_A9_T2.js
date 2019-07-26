













if (String(0.0000001) !== "1e-7") {
  $ERROR('#1: String(0.0000001) === "1e-7". Actual: ' + (String(0.0000001)));
}


if (String(0.000000000100000000000) !== "1e-10") {
  $ERROR('#2: String(0.000000000100000000000) === "1e-10". Actual: ' + (String(0.000000000100000000000)));
}


if (String(1e-7) !== "1e-7") {
  $ERROR('#3: String(1e-7) === "1e-7". Actual: ' + (String(1e-7)));
}


if (String(1.0e-10) !== "1e-10") {
  $ERROR('#4: String(1.0e-10) === "1e-10". Actual: ' + (String(1.0e-10)));
}


if (String(1E-7) !== "1e-7") {
  $ERROR('#5: String(1E-7) === "1e-7". Actual: ' + (String(1E-7)));
}


if (String(1.0E-10) !== "1e-10") {
  $ERROR('#6: String(1.0E-10) === "1e-10". Actual: ' + (String(1.0E-10)));
}


if (String(-0.0000001) !== "-1e-7") {
  $ERROR('#7: String(-0.0000001) === "1e-7". Actual: ' + (String(-0.0000001)));
}


if (String(-0.000000000100000000000) !== "-1e-10") {
  $ERROR('#8: String(-0.000000000100000000000) === "1e-10". Actual: ' + (String(-0.000000000100000000000)));
}


if (String(-1e-7) !== "-1e-7") {
  $ERROR('#9: String(-1e-7) === "-1e-7". Actual: ' + (String(-1e-7)));
}


if (String(-1.0e-10) !== "-1e-10") {
  $ERROR('#10: String(-1.0e-10) === "-1e-10". Actual: ' + (String(-1.0e-10)));
}


if (String(-1E-7) !== "-1e-7") {
  $ERROR('#11: String(-1E-7) === "-1e-7". Actual: ' + (String(-1E-7)));
}


if (String(-1.0E-10) !== "-1e-10") {
  $ERROR('#12: String(-1.0E-10) === "-1e-10". Actual: ' + (String(-1.0E-10)));
}

