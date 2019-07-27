


const NS_APP_USER_SEARCH_DIR  = "UsrSrchPlugns";

function run_test() {
  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();

  
  
  
  let dir = Services.dirsvc.get(NS_APP_USER_SEARCH_DIR, Ci.nsIFile);
  if (!dir.exists())
    dir.create(dir.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  do_get_file("data/engine-override.xml").copyTo(dir, "bug645970.xml");

  do_check_false(Services.search.isInitialized);

  
  let engines = Services.search.getEngines();
  do_check_eq(engines.length, 1);

  do_check_true(Services.search.isInitialized);

  
  let engine = Services.search.getEngineByName("bug645970");
  do_check_neq(engine, null);

  do_check_eq(engine.description, "bug645970");
}
