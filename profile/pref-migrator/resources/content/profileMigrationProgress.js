









































function onLoad(oldProfilePath, newProfilePath) {
  dump("start of pref migration\n");
  window.focus();
  var prefmigrator = Components.classes['@mozilla.org/profile/migration;1'].createInstance(Components.interfaces.nsIPrefMigration);
  if (prefmigrator) {
    try {
      prefmigrator.ProcessPrefsFromJS();
    }
    catch (ex) {
      dump("failed to migrate: ex="+ ex + "\n");
    }
  }
}
