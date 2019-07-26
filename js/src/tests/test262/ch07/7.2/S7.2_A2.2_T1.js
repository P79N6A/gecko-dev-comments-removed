










if (eval("'\u000Bstr\u000Bing\u000B'") !== "\u000Bstr\u000Bing\u000B") {
  $ERROR('#1: eval("\'\\u000Bstr\\u000Bing\\u000B\'") === "\\u000Bstr\\u000Bing\\u000B"');
}


if (eval("'\vstr\ving\v'") !== "\vstr\ving\v") {
  $ERROR('#2: eval("\'\\vstr\\ving\\v\'") === "\\vstr\\ving\\v"');
}

