










if ("1" + undefined !== "1undefined") {
  $ERROR('#1: "1" + undefined === "1undefined". Actual: ' + ("1" + undefined));
}


if (undefined + "1" !== "undefined1") {
  $ERROR('#2: undefined + "1" === "undefined1". Actual: ' + (undefined + "1"));
}


if (new String("1") + undefined !== "1undefined") {
  $ERROR('#3: new String("1") + undefined === "1undefined". Actual: ' + (new String("1") + undefined));
}


if (undefined + new String("1") !== "undefined1") {
  $ERROR('#4: undefined + new String("1") === "undefined1". Actual: ' + (undefined + new String("1")));
}

