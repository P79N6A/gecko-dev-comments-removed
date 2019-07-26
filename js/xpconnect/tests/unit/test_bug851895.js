const Cu = Components.utils;

function run_test() {
  
  var isXrayWrapper = Components.utils.isXrayWrapper;
  do_check_true(!isXrayWrapper({}), "Didn't throw");

  
  var isSuccessCode = Components.isSuccessCode;
  try { isSuccessCode(Components.results.NS_OK); } catch (e) {};
}
