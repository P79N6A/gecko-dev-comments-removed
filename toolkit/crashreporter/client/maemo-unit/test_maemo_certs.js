












function run_test() {
  let file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath("/bin/sh");

  let process = Components.classes["@mozilla.org/process/util;1"]
                        .createInstance(Components.interfaces.nsIProcess);
  process.init(file);

  let shscript = do_get_file("opensslverify.sh");
  let cacerts = do_get_file("crashreporter.crt");
  let servercert = do_get_file("crashreports.crt");
  let args = [shscript.path, cacerts.path, servercert.path];
  process.run(true, args, args.length);

  dump('If the following test fails, the logic in toolkit/crashreporter/client/certdata2pem.py needs to be fixed, otherwise crash report submission on Maemo will fail.\n');
  do_check_eq(process.exitValue, 0);
}
