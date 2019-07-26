










if (eval("'\u000Cstr\u000Cing\u000C'") !== "\u000Cstr\u000Cing\u000C") {
  $ERROR('#1: eval("\'\\u000Cstr\\u000Cing\\u000C\'") === "\\u000Cstr\\u000Cing\\u000C"');
}


if (eval("'\fstr\fing\f'") !== "\fstr\fing\f") {
  $ERROR('#2: eval("\'\\fstr\\fing\\f\'") === "\\fstr\\fing\\f"');
}

