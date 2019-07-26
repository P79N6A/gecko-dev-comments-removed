










if (true.toString() !== "true") {
  $ERROR('#1: true.toString() === "true". Actual: ' + (true.toString()));
}


if (false["toString"]() !== "false") {
  $ERROR('#2: false["toString"]() === "false". Actual: ' + (false["toString"]()));
}


if (new Boolean(true).toString() !== "true") {
  $ERROR('#3: new Boolean(true).toString() === "true". Actual: ' + (new Boolean(true).toString()));
}


if (new Boolean(false)["toString"]() !== "false") {
  $ERROR('#4: new Boolean(false)["toString"]() === "false". Actual: ' + (new Boolean(false)["toString"]()));
}  

