










if (eval("'\u0009str\u0009ing\u0009'") !== "\u0009str\u0009ing\u0009") {
  $ERROR('#1: eval("\'\\u0009str\\u0009ing\\u0009\'") === "\\u0009str\\u0009ing\\u0009"');
}


if (eval("'\tstr\ting\t'") !== "\tstr\ting\t") {
  $ERROR('#2: eval("\'\\tstr\\ting\\t\'") === "\\tstr\\ting\\t"');
}

