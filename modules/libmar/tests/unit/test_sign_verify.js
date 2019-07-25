


function run_test() {

  





  function signMAR(inMAR, outMAR) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let NSSConfigDir = do_get_file("data");
    let args = ["-d", NSSConfigDir.path, "-n", "mycert", "-s", 
                inMAR.path, outMAR.path];

    do_print('Running sign operation: ' + signmarBin.path);
    process.init(signmarBin);
    process.run(true, args, args.length);

    
    do_check_eq(process.exitValue, 0);
  }

  




  function verifyMAR(signedMAR, wantSuccess) {
    if (wantSuccess === undefined) {
      wantSuccess = true;
    }
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    let DERFile = do_get_file("data/mycert.der");

    
    let args;

    
    
    var isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);

    
    
    
    
    
    if (isWindows) {
      args = ["-D", DERFile.path, "-v", signedMAR.path];
    } else {
      let NSSConfigDir = do_get_file("data");
      args = ["-d", NSSConfigDir.path, "-n", "mycert", "-v", signedMAR.path];
    }

    do_print('Running verify operation: ' + signmarBin.path);
    process.init(signmarBin);
    try {
      
      process.run(true, args, args.length);
    } catch (e) {
      process.exitValue = -1;
    }

    
    if (wantSuccess) {
      do_check_eq(process.exitValue, 0);
    } else {
      do_check_neq(process.exitValue, 0);
    }
  }

  





  function stripMARSignature(signedMAR, outMAR) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let args = ["-r", signedMAR.path, outMAR.path];

    do_print('Running sign operation: ' + signmarBin.path);
    process.init(signmarBin);
    process.run(true, args, args.length);

    
    do_check_eq(process.exitValue, 0);
  }


  function cleanup() {
    let outMAR = do_get_file("signed_out.mar", true);
    if (outMAR.exists()) {
      outMAR.remove(false);
    }

    outMAR = do_get_file("out.mar", true);
    if (outMAR.exists()) {
      outMAR.remove(false);
    }

    let outDir = do_get_file("out", true);
    if (outDir.exists()) {
      outDir.remove(true);
    }
  }

  
  let tests = {
    
    test_sign: function() {
      let inMAR = do_get_file("data/binary_data_mar.mar");
      let outMAR = do_get_file("signed_out.mar", true);
      do_check_false(outMAR.exists());
      signMAR(inMAR, outMAR);
      do_check_true(outMAR.exists());
      let outMARData = getBinaryFileData(outMAR);
      let refMAR = do_get_file("data/signed_pib_mar.mar");
      let refMARData = getBinaryFileData(refMAR);
      compareBinaryData(outMARData, refMARData);
    }, 
    
    test_verify: function() {
      let signedMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(signedMAR);
    }, 
    
    test_verify_no_pib: function() {
      let signedMAR = do_get_file("data/signed_no_pib_mar.mar");
      verifyMAR(signedMAR);
    }, 
    
    
    test_crafted_mar: function() {
      let signedBadMAR = do_get_file("data/manipulated_signed_mar.mar");
      verifyMAR(signedBadMAR, false);
    }, 
    
    test_strip_signature: function() {
      let originalMAR = do_get_file("data/binary_data_mar.mar");
      let signedMAR = do_get_file("signed_out.mar");
      let outMAR = do_get_file("out.mar", true);
      stripMARSignature(signedMAR, outMAR);

      
      let outMARData = getBinaryFileData(outMAR);
      let originalMARData = getBinaryFileData(originalMAR);
      compareBinaryData(outMARData, originalMARData);
    },
  };

  cleanup();

  
  do_check_eq(run_tests(tests), 5);

  do_register_cleanup(cleanup);
}
