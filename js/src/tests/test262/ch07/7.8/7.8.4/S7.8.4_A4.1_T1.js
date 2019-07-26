










if (String.fromCharCode(0x0008) !== "\b") {
  $ERROR('#1: String.fromCharCode(0x0008) === "\\b"');
}


if (String.fromCharCode(0x0009) !== "\t") {
  $ERROR('#2: String.fromCharCode(0x0009) === "\\t"');
}


if (String.fromCharCode(0x000A) !== "\n") {
  $ERROR('#3: String.fromCharCode(0x000A) === "\\n"');
}


if (String.fromCharCode(0x000B) !== "\v") {
  $ERROR('#4: String.fromCharCode(0x000B) === "\\v"');
}


if (String.fromCharCode(0x000C) !== "\f") {
  $ERROR('#5: String.fromCharCode(0x000C) === "\\f"');
}


if (String.fromCharCode(0x000D) !== "\r") {
  $ERROR('#6: String.fromCharCode(0x000D) === "\\r"');
}

