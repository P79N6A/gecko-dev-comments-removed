





function run_test()
{
  var sb = Components.utils.Sandbox("http://www.blah.com");
  sb.prop = "prop"
  var refToObjFromSb = Components.utils.evalInSandbox("var a = {prop2:'prop2'}; a", sb); 
  Components.utils.nukeSandbox(sb);
  try{
    sb.prop;
    do_check_true(false);
  } catch (e) {
    do_check_true(e.toString().indexOf("can't access dead object") > -1);
  }
  
  try{
    refToObjFromSb.prop2;
    do_check_true(false);
  } catch (e) {
    do_check_true(e.toString().indexOf("can't access dead object") > -1);
  }
  
}
