


Cu.import("resource://services-common/utils.js");

function run_test() {
  let str = "Umlaute: \u00FC \u00E4\n"; 
  let encoded = CommonUtils.encodeUTF8(str);
  let decoded = CommonUtils.decodeUTF8(encoded);
  do_check_eq(decoded, str);
}
