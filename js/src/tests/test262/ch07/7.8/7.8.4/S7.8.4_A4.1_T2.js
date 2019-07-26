










if (String.fromCharCode(0x0027) !== "\'") {
  $ERROR('#1: String.fromCharCode(0x0027) === "\\\'"');
}


if (String.fromCharCode(0x0022) !== '\"') {
  $ERROR('#2: String.fromCharCode(0x0027) === \'\\\"\'');
}


if (String.fromCharCode(0x005C) !== "\\") {
  $ERROR('#3: String.fromCharCode(0x005C) === "\\\"');
}


if ("\'" !== "'") {
  $ERROR('#4: "\'" === "\\\'"');
}


if ('\"' !== '"') {
  $ERROR('#5: \'\"\' === \'\\\"\'');
}

