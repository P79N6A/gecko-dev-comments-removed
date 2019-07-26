










if ("1" + null !== "1null") {
  $ERROR('#1: "1" + null === "1null". Actual: ' + ("1" + null));
}


if (null + "1" !== "null1") {
  $ERROR('#2: null + "1" === "null1". Actual: ' + (null + "1"));
}


if (new String("1") + null !== "1null") {
  $ERROR('#3: new String("1") + null === "1null". Actual: ' + (new String("1") + null));
}


if (null + new String("1") !== "null1") {
  $ERROR('#4: null + new String("1") === "null1". Actual: ' + (null + new String("1")));
}

