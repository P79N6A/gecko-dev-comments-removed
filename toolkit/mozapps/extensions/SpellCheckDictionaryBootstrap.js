



































var hunspell, dir;

function startup(data) {
  hunspell = Components.classes["@mozilla.org/spellchecker/engine;1"]
                       .getService(Components.interfaces.mozISpellCheckingEngine);
  dir = data.installPath.clone();
  dir.append("dictionaries");
  hunspell.addDirectory(dir);
}

function shutdown() {
  hunspell.removeDirectory(dir);
}
