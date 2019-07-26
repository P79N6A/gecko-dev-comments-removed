










if (typeof this !== "object") {
  $ERROR('#1: typeof this === "object". Actual: ' + (typeof this));
}


if (typeof new Object() !== "object") {
  $ERROR('#2: typeof new Object() === "object". Actual: ' + (typeof new Object()));
}


if (typeof new Array(1,2,3) !== "object") {
  $ERROR('#3: typeof new Array(1,2,3) === "object". Actual: ' + (typeof new Array(1,2,3)));
}


if (typeof Array(1,2,3) !== "object") {
  $ERROR('#4: typeof Array(1,2,3) === "object". Actual: ' + (typeof Array(1,2,3)));
}


if (typeof new String("x") !== "object") {
  $ERROR('#5: typeof new String("x") === "object". Actual: ' + (typeof new String("x")));
}


if (typeof new Boolean(true) !== "object") {
  $ERROR('#6: typeof new Boolean(true) === "object". Actual: ' + (typeof new Boolean(true)));
}


if (typeof new Number(1) !== "object") {
  $ERROR('#7: typeof new Number(1) === "object". Actual: ' + (typeof new Number(1)));
}





if (typeof Math !== "object") {
  $ERROR('#8: typeof Math === "object". Actual: ' + (typeof Math));
}


if (typeof new Date() !== "object") {
  $ERROR('#9: typeof new Date() === "object". Actual: ' + (typeof new Date()));
}


if (typeof new Error() !== "object") {
  $ERROR('#10: typeof new Error() === "object". Actual: ' + (typeof new Error()));
}


if (typeof new RegExp() !== "object") {
  $ERROR('#11: typeof new RegExp() === "object". Actual: ' + (typeof new RegExp()));
}


if (typeof RegExp() !== "object") {
  $ERROR('#12: typeof RegExp() === "object". Actual: ' + (typeof RegExp()));
}

