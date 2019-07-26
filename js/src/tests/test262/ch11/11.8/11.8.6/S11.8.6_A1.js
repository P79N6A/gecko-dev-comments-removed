










if (eval("({})\u0009instanceof\u0009Object") !== true) {
  $ERROR('#1: ({})\\u0009instanceof\\u0009Object === true');
}


if (eval("({})\u000Binstanceof\u000BObject") !== true) {
  $ERROR('#2: ({})\\u000Binstanceof\\u000BObject === true');  
}


if (eval("({})\u000Cinstanceof\u000CObject") !== true) {
  $ERROR('#3: ({})\\u000Cinstanceof\\u000CObject === true');
}


if (eval("({})\u0020instanceof\u0020Object") !== true) {
  $ERROR('#4: ({})\\u0020instanceof\\u0020Object === true');
}


if (eval("({})\u00A0instanceof\u00A0Object") !== true) {
  $ERROR('#5: ({})\\u00A0instanceof\\u00A0Object === true');
}


if (eval("({})\u000Ainstanceof\u000AObject") !== true) {
  $ERROR('#6: ({})\\u000Ainstanceof\\u000AObject === true');  
}


if (eval("({})\u000Dinstanceof\u000DObject") !== true) {
  $ERROR('#7: ({})\\u000Dinstanceof\\u000DObject === true');
}


if (eval("({})\u2028instanceof\u2028Object") !== true) {
  $ERROR('#8: ({})\\u2028instanceof\\u2028Object === true');
}


if (eval("({})\u2029instanceof\u2029Object") !== true) {
  $ERROR('#9: ({})\\u2029instanceof\\u2029Object === true');
}


if (eval("({})\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029instanceof\u0009\u000B\u000C\u0020\u00A0\u000A\u000D\u2028\u2029Object") !== true) {
  $ERROR('#10: ({})\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029instanceof\\u0009\\u000B\\u000C\\u0020\\u00A0\\u000A\\u000D\\u2028\\u2029Object === true');
}

