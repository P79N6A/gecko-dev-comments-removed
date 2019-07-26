











if (!("" === "")) {
  $ERROR('#1: "" === ""');
}


if (!(" " === " ")) {
  $ERROR('#2: " " === " "');
}


if (!("string" === "string")) {
  $ERROR('#3: "string" === "string"');
}


if (" string" === "string ") {
  $ERROR('#4: " string" !== "string "');
}


if ("1.0" === "1") {
  $ERROR('#5: "1.0" !== "1"');
}

