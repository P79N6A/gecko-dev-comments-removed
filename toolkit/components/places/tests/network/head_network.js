







































do_load_httpd_js();

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



