




function check_state(aTag, aExpectedClicktoplay, aExpectedDisabled) {
  do_check_eq(aTag.clicktoplay, aExpectedClicktoplay);
  do_check_eq(aTag.disabled, aExpectedDisabled);
}

function run_test() {
  let tag = get_test_plugintag();
  tag.enabledState = Ci.nsIPluginTag.STATE_ENABLED;
  check_state(tag, false, false);

  
  tag.enabledState = Ci.nsIPluginTag.STATE_CLICKTOPLAY;
  check_state(tag, true, false);
  tag.enabledState = Ci.nsIPluginTag.STATE_ENABLED;
  check_state(tag, false, false);

  
  tag.enabledState = Ci.nsIPluginTag.STATE_DISABLED;
  check_state(tag, false, true);
  tag.enabledState = Ci.nsIPluginTag.STATE_ENABLED;
  check_state(tag, false, false);

  
  tag.enabledState = Ci.nsIPluginTag.STATE_DISABLED;
  check_state(tag, false, true);
  tag.enabledState = Ci.nsIPluginTag.STATE_CLICKTOPLAY;
  check_state(tag, true, false);
  tag.enabledState = Ci.nsIPluginTag.STATE_DISABLED;
  check_state(tag, false, true);

  
  tag.enabledState = Ci.nsIPluginTag.STATE_ENABLED;
  check_state(tag, false, false);
}
