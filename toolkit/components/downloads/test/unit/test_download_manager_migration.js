








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get("ProfD", Ci.nsIFile);
} catch (e) { }
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfD") {
        return dirSvc.get("CurProcD", Ci.nsILocalFile);
      } else if (prop == "DLoads") {
        var file = dirSvc.get("CurProcD", Ci.nsILocalFile);
        file.append("downloads.rdf");
        return file;
      }
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

function cleanup()
{
  
  var rdfFile = dirSvc.get("DLoads", Ci.nsIFile);
  if (rdfFile.exists()) rdfFile.remove(true);
  
  
  var dbFile = dirSvc.get("ProfD", Ci.nsIFile);
  dbFile.append("downloads.sqlite");
  if (dbFile.exists()) dbFile.remove(true);
}

cleanup();


var file = do_get_file("toolkit/components/downloads/test/unit/downloads.rdf");
var newFile = dirSvc.get("ProfD", Ci.nsIFile);
file.copyTo(newFile, "downloads.rdf");

const nsIDownloadManager = Ci.nsIDownloadManager;
const dm = Cc["@mozilla.org/download-manager;1"].getService(nsIDownloadManager);

function test_count_entries()
{
  var stmt = dm.DBConnection.createStatement("SELECT COUNT(*) " +
                                             "FROM moz_downloads");
  stmt.executeStep();

  do_check_eq(7, stmt.getInt32(0));

  stmt.reset();
}

function test_random_download()
{
  var stmt = dm.DBConnection.createStatement("SELECT COUNT(*), source, target," +
                                           "  iconURL, state " +
                                           "FROM moz_downloads " +
                                           "WHERE name = ?1");
  stmt.bindStringParameter(0, "Firefox 2.0.0.3.dmg");
  stmt.executeStep();

  do_check_eq(1, stmt.getInt32(0));
  do_check_eq("http://ftp-mozilla.netscape.com/pub/mozilla.org/firefox/releases/2.0.0.3/mac/en-US/Firefox%202.0.0.3.dmg", stmt.getUTF8String(1));
  do_check_eq("file:///Users/sdwilsh/Desktop/Firefox 2.0.0.3.dmg", stmt.getUTF8String(2));
  do_check_eq("", stmt.getString(3));
  do_check_eq(1, stmt.getInt32(4));

  stmt.reset();
}


function test_dm_cleanup()
{
  dm.cleanUp();

  var stmt = dm.DBConnection.createStatement("SELECT COUNT(*) " +
                                             "FROM moz_downloads");
  stmt.executeStep();

  do_check_eq(0, stmt.getInt32(0));

  stmt.reset();
}

var tests = [test_count_entries, test_random_download, test_dm_cleanup];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
  
  cleanup();
}
