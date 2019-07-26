











if (Object("some string").valueOf() !== "some string"){
  $ERROR('#1: Object("some string").valueOf() === "some string". Actual: ' + (Object("some string").valueOf()));
}


if (typeof Object("some string") !== "object"){
  $ERROR('#2: typeof Object("some string") === "object". Actual: ' + (typeof Object("some string")));
}


if (Object("some string").constructor.prototype !== String.prototype){
  $ERROR('#3: Object("some string").constructor.prototype === String.prototype. Actual: ' + (Object("some string").constructor.prototype));
}


if (Object("").valueOf() !== ""){
  $ERROR('#4: Object("").valueOf() === false. Actual: ' + (Object("").valueOf()));
}


if (typeof Object("") !== "object"){
  $ERROR('#5: typeof Object("") === "object". Actual: ' + (typeof Object("")));
}


if (Object("").constructor.prototype !== String.prototype){
  $ERROR('#6: Object("").constructor.prototype === String.prototype. Actual: ' + (Object("").constructor.prototype));
}


if (Object("\r\t\b\n\v\f").valueOf() !== "\r\t\b\n\v\f"){
  $ERROR('#7: Object("\\r\\t\\b\\n\\v\\f").valueOf() === false. Actual: ' + (Object("\r\t\b\n\v\f").valueOf()));
}


if (typeof Object("\r\t\b\n\v\f") !== "object"){
  $ERROR('#8: typeof Object("\\r\\t\\b\\n\\v\\f") === "object". Actual: ' + (typeof Object("\r\t\b\n\v\f")));
}


if (Object("\r\t\b\n\v\f").constructor.prototype !== String.prototype){
  $ERROR('#9: Object("\\r\\t\\b\\n\\v\\f").constructor.prototype === String.prototype. Actual: ' + (Object("\r\t\b\n\v\f").constructor.prototype));
}


if (Object(String(10)).valueOf() !== "10"){
  $ERROR('#10: Object(String(10)).valueOf() === false. Actual: ' + (Object(String(10)).valueOf()));
}


if (typeof Object(String(10)) !== "object"){
  $ERROR('#11: typeof Object(String(10)) === "object". Actual: ' + (typeof Object(String(10))));
}


if (Object(String(10)).constructor.prototype !== String.prototype){
  $ERROR('#12: Object(String(10)).constructor.prototype === String.prototype. Actual: ' + (Object(String(10)).constructor.prototype));
}

