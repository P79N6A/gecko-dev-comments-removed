










var identifier = String.fromCharCode(0x005F);
eval("var " + identifier + "=1");
if (eval(identifier + "===1") !== true) {
  $ERROR('#1: var identifier = String.fromCharCode(0x005F); eval("var " + identifier + "=1"); eval(identifier + "===1") === true');
}


if ("_" !== String.fromCharCode(0x005F)) {
  $ERROR('#2: "_" === String.fromCharCode(0x005F)');
}

