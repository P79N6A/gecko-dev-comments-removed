













if (String(0.1) !== "0.1") {
  $ERROR('#1: String(0.1) === "0.1". Actual: ' + (String(0.1)));
}


if (String(0.000001) !== "0.000001") {
  $ERROR('#2: String(0.000001) === "0.000001". Actual: ' + (String(0.000001)));
}


if (String(1e-6) !== "0.000001") {
  $ERROR('#3: String(1e-6) === "0.000001". Actual: ' + (String(1e-6)));
}


if (String(1E-6) !== "0.000001") {
  $ERROR('#4: String(1E-6) === "0.000001". Actual: ' + (String(1E-6)));
}


if (String(-0.1) !== "-0.1") {
  $ERROR('#5: String(-0.1) === "-0.1". Actual: ' + (String(-0.1)));
}


if (String(-0.000001) !== "-0.000001") {
  $ERROR('#6: String(-0.000001) === "-0.000001". Actual: ' + (String(-0.000001)));
}


if (String(-1e-6) !== "-0.000001") {
  $ERROR('#7: String(-1e-6) === "0.000001". Actual: ' + (String(-1e-6)));
}


if (String(-1E-6) !== "-0.000001") {
  $ERROR('#8: String(-1E-6) === "0.000001". Actual: ' + (String(-1E-6)));
}


