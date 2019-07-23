function run_test() {
  
  
  
  var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                     .getService();

  var prompt;

  prompt = ww.nsIWindowWatcher.getNewPrompter(null);
  do_check_neq(prompt, null);
  prompt = ww.nsIWindowWatcher.getNewAuthPrompter(null);
  do_check_neq(prompt, null);

  prompt = ww.nsIPromptFactory.getPrompt(null,
                                         Components.interfaces.nsIPrompt);
  do_check_neq(prompt, null);
  prompt = ww.nsIPromptFactory.getPrompt(null,
                                         Components.interfaces.nsIAuthPrompt);
  do_check_neq(prompt, null);
  prompt = ww.nsIPromptFactory.getPrompt(null,
                                         Components.interfaces.nsIAuthPrompt2);
  do_check_neq(prompt, null);
}
