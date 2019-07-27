


const Cc = Components.classes;
const Ci = Components.interfaces;

const isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
const refMARPrefix = (isWindows ? "win_" : "");
const BIN_SUFFIX = (isWindows ? ".exe" : "");

let tempDir = do_get_tempdir();








function compareBinaryData(arr1, arr2) {
  do_check_eq(arr1.length, arr2.length);
  for (let i = 0; i < arr1.length; i++) {
    if (arr1[i] != arr2[i]) {
      throw "Data differs at index " + i + 
            ", arr1: " + arr1[i] + ", arr2: " + arr2[i];
    }
  }
}







function getBinaryFileData(file) {
  let fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
                   createInstance(Ci.nsIFileInputStream);
  
  fileStream.init(file, -1, -1, null);

  
  let stream = Cc["@mozilla.org/binaryinputstream;1"].
               createInstance(Ci.nsIBinaryInputStream);
  stream.setInputStream(fileStream);
  let bytes = stream.readByteArray(stream.available());
  fileStream.close();
  return bytes;
}









function run_tests(obj) {
  let cleanup_per_test = obj.cleanup_per_test;
  if (cleanup_per_test === undefined) {
    cleanup_per_test = function() {};
  }

  do_register_cleanup(cleanup_per_test);

  
  cleanup_per_test();

  let ranCount = 0;
  
  for (let f in obj) {
    if (typeof obj[f] === "function" && 
        obj.hasOwnProperty(f) &&
        f.toString().indexOf("test_") === 0) {
      obj[f]();
      cleanup_per_test();
      ranCount++;
    }
  }
  return ranCount;
}








function createMAR(outMAR, dataDir, files) {
  
  do_check_true(files.length > 0);

  
  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

  
  do_check_true(signmarBin.exists());
  do_check_true(signmarBin.isExecutable());

  
  
  
  
  
  for (filePath of files) {
    let f = dataDir.clone();
    f.append(filePath);
    f.permissions = 0664;
  }

  
  let args = ["-C", dataDir.path, "-H", "\@MAR_CHANNEL_ID\@", 
              "-V", "13.0a1", "-c", outMAR.path];
  args = args.concat(files);

  do_print('Running: ' + signmarBin.path);
  process.init(signmarBin);
  process.run(true, args, args.length);

  
  do_check_eq(process.exitValue, 0);

  
  do_check_true(outMAR.exists());
}







function extractMAR(mar, dataDir) {
  
  let process = Cc["@mozilla.org/process/util;1"].
                createInstance(Ci.nsIProcess);
  let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

  
  do_check_true(signmarBin.exists());
  do_check_true(signmarBin.isExecutable());

  
  let args = ["-C", dataDir.path, "-x", mar.path];

  do_print('Running: ' + signmarBin.path);
  process.init(signmarBin);
  process.run(true, args, args.length);

  
  do_check_eq(process.exitValue, 0);
}


