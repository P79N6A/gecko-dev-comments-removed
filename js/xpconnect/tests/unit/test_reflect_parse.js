





















function run_test() {
  
  var parseData = Reflect.parse('"use strict"');
  do_check_eq(parseData.body[0].expression.value, "use strict");
}
