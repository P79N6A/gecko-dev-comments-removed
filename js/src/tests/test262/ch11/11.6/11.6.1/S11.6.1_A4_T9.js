










if (-Number.MAX_VALUE + Number.MAX_VALUE + Number.MAX_VALUE !== (-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE) {
  $ERROR('#1: -Number.MAX_VALUE + Number.MAX_VALUE + Number.MAX_VALUE === (-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE. Actual: ' + (-Number.MAX_VALUE + Number.MAX_VALUE + Number.MAX_VALUE));
} 


if ((-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE === -Number.MAX_VALUE + (Number.MAX_VALUE + Number.MAX_VALUE)) {
  $ERROR('#2: (-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE === -Number.MAX_VALUE + (Number.MAX_VALUE + Number.MAX_VALUE). Actual: ' + ((-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE));
}


if ("1" + 1 + 1 !== ("1" + 1) + 1) {
  $ERROR('#3: "1" + 1 + 1 === ("1" + 1) + 1. Actual: ' + ("1" + 1 + 1));
} 


if (("1" + 1) + 1 === "1" + (1 + 1)) {
  $ERROR('#4: ("1" + 1) + 1 !== "1" + (1 + 1)');
}

