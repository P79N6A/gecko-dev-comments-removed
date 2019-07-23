

const Cc = Components.classes;
const Ci = Components.interfaces;
const path = "modules/libjar/test/unit/data/test_bug379841.zip";

const MAX_TIME_DIFF = 2000000;

var ENTRY_NAME = "test";

var ENTRY_TIME = new Date(2007, 4, 7, 14, 35, 49, 0);

function run_test() {
  var file = do_get_file(path);
  var zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipReader.open(file);
  var entry = zipReader.getEntry(ENTRY_NAME);
  var diff = Math.abs(entry.lastModifiedTime - ENTRY_TIME.getTime()*1000);
  zipReader.close();
  if (diff >= MAX_TIME_DIFF)
    do_throw(diff);
}
