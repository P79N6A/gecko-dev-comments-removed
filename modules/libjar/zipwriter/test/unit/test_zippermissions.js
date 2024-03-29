




const DATA = "ZIP WRITER TEST DATA";

var TESTS = [];

function build_tests() {
  var id = 0;

  
  for (let u = 4; u <= 7; u++) {
    for (let g = 0; g <= 7; g++) {
      for (let o = 0; o <= 7; o++) {
        TESTS[id] = {
          name: "test" + u + g + o,
          permission: (u << 6) + (g << 3) + o
        };
        id++;
      }
    }
  }
}

function run_test() {
  build_tests();

  var foStream = Cc["@mozilla.org/network/file-output-stream;1"].
                 createInstance(Ci.nsIFileOutputStream);

  var tmp = tmpDir.clone();
  tmp.append("temp-permissions");
  tmp.createUnique(Ci.nsILocalFile.DIRECTORY_TYPE, 0755);

  var file = tmp.clone();
  file.append("tempfile");

  zipW.open(tmpFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  for (let i = 0; i < TESTS.length; i++) {
    
    
    foStream.init(file, 0x02 | 0x08 | 0x20, TESTS[i].permission, 0);
    foStream.close();

    
    
    var perm = file.permissions & 0xfff;
    if (TESTS[i].permission != perm) {
      dump("File permissions for " + TESTS[i].name + " were " + perm.toString(8) + "\n");
      TESTS[i].permission = perm;
    }

    zipW.addEntryFile(TESTS[i].name, Ci.nsIZipWriter.COMPRESSION_NONE, file, false);
    do_check_eq(zipW.getEntry(TESTS[i].name).permissions, TESTS[i].permission | 0o400);
    file.permissions = 0o600;
    file.remove(true);
  }
  zipW.close();

  zipW.open(tmpFile, PR_RDWR);
  for (let i = 0; i < TESTS.length; i++) {
    dump("Testing zipwriter file permissions for " + TESTS[i].name + "\n");
    do_check_eq(zipW.getEntry(TESTS[i].name).permissions, TESTS[i].permission | 0o400);
  }
  zipW.close();

  var zipR = new ZipReader(tmpFile);
  for (let i = 0; i < TESTS.length; i++) {
    dump("Testing zipreader file permissions for " + TESTS[i].name + "\n");
    do_check_eq(zipR.getEntry(TESTS[i].name).permissions, TESTS[i].permission | 0o400);
    dump("Testing extracted file permissions for " + TESTS[i].name + "\n");
    zipR.extract(TESTS[i].name, file);
    do_check_eq(file.permissions & 0xfff, TESTS[i].permission);
    do_check_false(file.isDirectory());
    file.permissions = 0o600;
    file.remove(true);
  }
  zipR.close();

  tmp.remove(true);
}
