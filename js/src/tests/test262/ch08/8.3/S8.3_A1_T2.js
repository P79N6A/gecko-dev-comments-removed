











if (typeof(true) !== "boolean") {
  $ERROR('#1: typeof(true) === "boolean"');
}





if (typeof(true) != "boolean") {
  $ERROR('#2: typeof(true) == "boolean"');
}





if (typeof(false) !== "boolean") {
  $ERROR('#3: typeof(false) === "boolean"');
}





if (typeof(false) != "boolean") {
  $ERROR('#4: typeof(false) == "boolean"');
}





if (true === false) {
  $ERROR('#5: true !== false');
}





if (true == false) {
  $ERROR('#6: true != false');
}





if (false === true) {
  $ERROR('#7: false !== true');
}





if (false == true) {
  $ERROR('#8: false != true');
}




