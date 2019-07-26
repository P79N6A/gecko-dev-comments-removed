











if (Object(true).valueOf() !== true){
  $ERROR('#1: Object(true).valueOf() === true. Actual: ' + (Object(true).valueOf()));
}


if (typeof Object(true) !== "object"){
  $ERROR('#2: typeof Object(true) === "object". Actual: ' + (typeof Object(true)));
}


if (Object(true).constructor.prototype !== Boolean.prototype){
  $ERROR('#3: Object(true).constructor.prototype === Boolean.prototype. Actual: ' + (Object(true).constructor.prototype));
}


if (Object(false).valueOf() !== false){
  $ERROR('#4: Object(false).valueOf() === false. Actual: ' + (Object(false).valueOf()));
}


if (typeof Object(false) !== "object"){
  $ERROR('#5: typeof Object(false) === "object". Actual: ' + (typeof Object(false)));
}


if (Object(false).constructor.prototype !== Boolean.prototype){
  $ERROR('#6: Object(false).constructor.prototype === Boolean.prototype. Actual: ' + (Object(false).constructor.prototype));
}

