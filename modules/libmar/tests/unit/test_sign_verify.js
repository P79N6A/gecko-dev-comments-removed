


function run_test() {

  





  function signMAR(inMAR, outMAR, certs, wantSuccess, useShortHandCmdLine) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let NSSConfigDir = do_get_file("data");
    let args = ["-d", NSSConfigDir.path];
    if (certs.length == 1 && useShortHandCmdLine) {
      args.push("-n", certs[0]);
    } else {
      for (i = 0; i < certs.length; i++) {
        args.push("-n" + i, certs[i]);
      }
    }
    args.push("-s", inMAR.path, outMAR.path);

    process.init(signmarBin);
    try {
      process.run(true, args, args.length);
    } catch(e) {
      
      process.exitValue = -1;
    }

    
    if (wantSuccess) {
      do_check_eq(process.exitValue, 0);
    } else {
      do_check_neq(process.exitValue, 0);
    }
  }


  







  function extractMARSignature(inMAR, sigIndex, extractedSig, wantSuccess) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let args = ["-n" + sigIndex, "-X", inMAR.path, extractedSig.path];

    process.init(signmarBin);
    try {
      process.run(true, args, args.length);
    } catch(e) {
      
      process.exitValue = -1;
    }

    
    if (wantSuccess) {
      do_check_eq(process.exitValue, 0);
    } else {
      do_check_neq(process.exitValue, 0);
    }
  }

  









  function importMARSignature(inMAR, sigIndex, sigFile, outMAR, wantSuccess) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let args = ["-n" + sigIndex, "-I", inMAR.path, sigFile.path, outMAR.path];

    process.init(signmarBin);
    try {
      process.run(true, args, args.length);
    } catch(e) {
      
      process.exitValue = -1;
    }

    
    if (wantSuccess) {
      do_check_eq(process.exitValue, 0);
    } else {
      do_check_neq(process.exitValue, 0);
    }
  }

  




  function verifyMAR(signedMAR, wantSuccess, certs, useShortHandCmdLine) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let args = [];

    
    
    var isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);

    
    
    
    
    
    if (isWindows) {
      if (certs.length == 1 && useShortHandCmdLine) {
        args.push("-D", "data/" + certs[0] + ".der");
      } else {
        for (i = 0; i < certs.length; i++) {
          args.push("-D" + i, "data/" + certs[i] + ".der");
        }
      }
      args.push("-v", signedMAR.path);
    } else {
      let NSSConfigDir = do_get_file("data");
      args = ["-d", NSSConfigDir.path];
      if (certs.length == 1 && useShortHandCmdLine) {
        args.push("-n", certs[0]);
      } else {
        for (i = 0; i < certs.length; i++) {
          args.push("-n" + i, certs[i]);
        }
      }
      args.push("-v", signedMAR.path);
    }

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

  





  function stripMARSignature(signedMAR, outMAR, wantSuccess) {
    
    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let signmarBin = do_get_file("signmar" + BIN_SUFFIX);

    
    do_check_true(signmarBin.exists());
    do_check_true(signmarBin.isExecutable());

    
    let args = ["-r", signedMAR.path, outMAR.path];

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


  function cleanup() {
    let outMAR = do_get_file("signed_out.mar", true);
    if (outMAR.exists()) {
      outMAR.remove(false);
    }
    outMAR = do_get_file("multiple_signed_out.mar", true);
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

  const wantFailure = false;
  const wantSuccess = true;
  
  let tests = {
    
    test_sign_single: function() {
      let inMAR = do_get_file("data/" + refMARPrefix + "binary_data_mar.mar");
      let outMAR = do_get_file("signed_out.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }
      signMAR(inMAR, outMAR, ["mycert"], wantSuccess, true);
      do_check_true(outMAR.exists());
      let outMARData = getBinaryFileData(outMAR);
      let refMAR = do_get_file("data/" + refMARPrefix + "signed_pib_mar.mar");
      let refMARData = getBinaryFileData(refMAR);
      compareBinaryData(outMARData, refMARData);
    }, 
    
    test_sign_multiple: function() {
      let inMAR = do_get_file("data/" + refMARPrefix + "binary_data_mar.mar");
      let outMAR = do_get_file("multiple_signed_out.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }
      do_check_false(outMAR.exists());
      signMAR(inMAR, outMAR, ["mycert", "mycert2", "mycert3"],
              wantSuccess, true);
      do_check_true(outMAR.exists());
      let outMARData = getBinaryFileData(outMAR);
      let refMAR = do_get_file("data/" + refMARPrefix + "multiple_signed_pib_mar.mar");
      let refMARData = getBinaryFileData(refMAR);
      compareBinaryData(outMARData, refMARData);
    },
    
    test_verify_single: function() {
      let signedMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(signedMAR, wantSuccess, ["mycert"], true);
      verifyMAR(signedMAR, wantSuccess, ["mycert"], false);
    }, 
    
    
    
    test_verify_single_too_many_certs: function() {
      let signedMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(signedMAR, wantFailure, ["mycert", "mycert"], true);
      verifyMAR(signedMAR, wantFailure, ["mycert", "mycert"], false);
    },
    
    test_verify_single_wrong_cert: function() {
      let signedMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(signedMAR, wantFailure, ["mycert2"], true);
      verifyMAR(signedMAR, wantFailure, ["mycert2"], false);
    },
    
    test_verify_multiple: function() {
      let signedMAR = do_get_file("data/multiple_signed_pib_mar.mar");
      verifyMAR(signedMAR, wantSuccess, ["mycert", "mycert2", "mycert3"]);
    },
    
    test_verify_unsigned_mar_file_fails: function() {
      let unsignedMAR = do_get_file("data/binary_data_mar.mar");
      verifyMAR(unsignedMAR, wantFailure, ["mycert", "mycert2", "mycert3"]);
    },
    
    
    
    
    test_verify_multiple_same_cert: function() {
      let signedMAR = do_get_file("data/multiple_signed_pib_mar.mar");
      verifyMAR(signedMAR, wantFailure, ["mycert", "mycert", "mycert"]);
    },
    
    
    test_verify_multiple_wrong_order: function() {
      let signedMAR = do_get_file("data/multiple_signed_pib_mar.mar");
      verifyMAR(signedMAR, wantSuccess, ["mycert", "mycert2", "mycert3"]);
      verifyMAR(signedMAR, wantFailure, ["mycert", "mycert3", "mycert2"]);
      verifyMAR(signedMAR, wantFailure, ["mycert2", "mycert", "mycert3"]);
      verifyMAR(signedMAR, wantFailure, ["mycert2", "mycert3", "mycert"]);
      verifyMAR(signedMAR, wantFailure, ["mycert3", "mycert", "mycert2"]);
      verifyMAR(signedMAR, wantFailure, ["mycert3", "mycert2", "mycert"]);
    },
    
    test_verify_no_pib: function() {
      let signedMAR = do_get_file("data/signed_no_pib_mar.mar");
      verifyMAR(signedMAR, wantSuccess, ["mycert"], true);
      verifyMAR(signedMAR, wantSuccess, ["mycert"], false);
    }, 
    
    test_verify_no_pib_multiple: function() {
      let signedMAR = do_get_file("data/multiple_signed_no_pib_mar.mar");
      verifyMAR(signedMAR, wantSuccess, ["mycert", "mycert2", "mycert3"]);
    },
    
    
    test_crafted_mar: function() {
      let signedBadMAR = do_get_file("data/manipulated_signed_mar.mar");
      verifyMAR(signedBadMAR, wantFailure, ["mycert"], true);
      verifyMAR(signedBadMAR, wantFailure, ["mycert"], false);
    }, 
    
    test_bad_path_verify_fails: function() {
      let noMAR = do_get_file("data/does_not_exist_.mar", true);
      do_check_false(noMAR.exists());
      verifyMAR(noMAR, wantFailure, ["mycert"], true);
    },
    
    test_strip_signature: function() {
      let originalMAR = do_get_file("data/" + 
                                    refMARPrefix + 
                                    "binary_data_mar.mar");
      let signedMAR = do_get_file("signed_out.mar");
      let outMAR = do_get_file("out.mar", true);
      stripMARSignature(signedMAR, outMAR, wantSuccess);

      
      let outMARData = getBinaryFileData(outMAR);
      let originalMARData = getBinaryFileData(originalMAR);
      compareBinaryData(outMARData, originalMARData);
    },
    
    test_strip_multiple_signatures: function() {
      let originalMAR = do_get_file("data/" +
                                    refMARPrefix +
                                    "binary_data_mar.mar");
      let signedMAR = do_get_file("multiple_signed_out.mar");
      let outMAR = do_get_file("out.mar", true);
      stripMARSignature(signedMAR, outMAR, wantSuccess);

      
      let outMARData = getBinaryFileData(outMAR);
      let originalMARData = getBinaryFileData(originalMAR);
      compareBinaryData(outMARData, originalMARData);
    },
    
    test_extract_sig_single: function() {
      let inMAR = do_get_file("data/signed_pib_mar.mar");
      let extractedSig = do_get_file("extracted_signature", true);
      if (extractedSig.exists()) {
        extractedSig.remove(false);
      }
      extractMARSignature(inMAR, 0, extractedSig, wantSuccess);
      do_check_true(extractedSig.exists());

      let referenceSig = do_get_file("data/signed_pib_mar.signature.0"); +
      compareBinaryData(extractedSig, referenceSig);
    },
    
    
    test_extract_sig_multi: function() {
      for (let i = 0; i < 3; i++) {
        let inMAR = do_get_file("data/multiple_signed_pib_mar.mar");
        let extractedSig = do_get_file("extracted_signature", true);
        if (extractedSig.exists()) {
          extractedSig.remove(false);
        }
        extractMARSignature(inMAR, i, extractedSig, wantSuccess);
        do_check_true(extractedSig.exists());

        let referenceSig = do_get_file("data/multiple_signed_pib_mar.sig." + i); +
        compareBinaryData(extractedSig, referenceSig);
      }
    },
    
    test_extract_sig_out_of_range: function() {
      let inMAR = do_get_file("data/signed_pib_mar.mar");
      let extractedSig = do_get_file("extracted_signature", true);
      if (extractedSig.exists()) {
        extractedSig.remove(false);
      }
      const outOfBoundsIndex = 5;
      extractMARSignature(inMAR, outOfBoundsIndex, extractedSig, wantFailure);
      do_check_false(extractedSig.exists());
    },
    
    test_bad_path_sign_fails: function() {
      let inMAR = do_get_file("data/does_not_exist_.mar", true);
      let outMAR = do_get_file("signed_out.mar", true);
      do_check_false(inMAR.exists());
      signMAR(inMAR, outMAR, ["mycert"], wantFailure, true);
      do_check_false(outMAR.exists());
    },
    
    
    
    test_verify_multiple_subset: function() {
      let signedMAR = do_get_file("data/multiple_signed_pib_mar.mar");
      verifyMAR(signedMAR, wantFailure, ["mycert", "mycert2"]);
    },
    
    
    test_import_sig_single: function() {
      
      let inMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(inMAR, wantSuccess, ["mycert"], false);
      verifyMAR(inMAR, wantFailure, ["mycert2"], false);
      verifyMAR(inMAR, wantFailure, ["mycert3"], false);

      
      let sigFile = do_get_file("data/signed_pib_mar.signature.mycert2");
      do_check_true(sigFile.exists());
      let outMAR = do_get_file("data/sigchanged_signed_pib_mar.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }

      
      importMARSignature(inMAR, 0, sigFile, outMAR, wantSuccess);

      
      
      do_check_true(outMAR.exists());
      verifyMAR(outMAR, wantFailure, ["mycert"], false);
      verifyMAR(outMAR, wantSuccess, ["mycert2"], false);
      verifyMAR(outMAR, wantFailure, ["mycert3"], false);

      
      
      let refMAR = do_get_file("data/signed_pib_mar_with_mycert2.mar");
      do_check_true(refMAR.exists());
      let refMARData = getBinaryFileData(refMAR);
      let outMARData = getBinaryFileData(outMAR);
      compareBinaryData(outMARData, refMARData);
    },
    
    
    test_import_wrong_sig: function() {
      
      let inMAR = do_get_file("data/signed_pib_mar.mar");
      verifyMAR(inMAR, wantSuccess, ["mycert"], false);
      verifyMAR(inMAR, wantFailure, ["mycert2"], false);
      verifyMAR(inMAR, wantFailure, ["mycert3"], false);

      
      let sigFile = do_get_file("data/multiple_signed_pib_mar.sig.0");
      do_check_true(sigFile.exists());
      let outMAR = do_get_file("data/sigchanged_signed_pib_mar.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }

      
      importMARSignature(inMAR, 0, sigFile, outMAR, wantSuccess);

      
      
      do_check_true(outMAR.exists());
      verifyMAR(outMAR, wantFailure, ["mycert"], false);
      verifyMAR(outMAR, wantFailure, ["mycert2"], false);
      verifyMAR(outMAR, wantFailure, ["mycert3"], false);
    },
    
    
    test_import_sig_multiple: function() {
      
      let inMAR = do_get_file("data/multiple_signed_pib_mar.mar");
      verifyMAR(inMAR, wantSuccess, ["mycert", "mycert2", "mycert3"], false);
      verifyMAR(inMAR, wantFailure, ["mycert", "mycert", "mycert3"], false);

      
      let sigFile = do_get_file("data/multiple_signed_pib_mar.sig.0");
      do_check_true(sigFile.exists());
      let outMAR = do_get_file("data/sigchanged_signed_pib_mar.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }

      
      const secondSigPos = 1;
      importMARSignature(inMAR, secondSigPos, sigFile, outMAR, wantSuccess);

      
      
      do_check_true(outMAR.exists());
      verifyMAR(outMAR, wantSuccess, ["mycert", "mycert", "mycert3"], false);
      verifyMAR(outMAR, wantFailure, ["mycert", "mycert2", "mycert3"], false);

      
      
      let refMAR = do_get_file("data/multiple_signed_pib_mar_2.mar");
      do_check_true(refMAR.exists());
      let refMARData = getBinaryFileData(refMAR);
      let outMARData = getBinaryFileData(outMAR);
      compareBinaryData(outMARData, refMARData);
    },
    
    test_bad_path_strip_fails: function() {
      let noMAR = do_get_file("data/does_not_exist_mar", true);
      do_check_false(noMAR.exists());
      let outMAR = do_get_file("out.mar", true);
      stripMARSignature(noMAR, outMAR, wantFailure);
    },
    
    test_extract_bad_path: function() {
      let noMAR = do_get_file("data/does_not_exist.mar", true);
      let extractedSig = do_get_file("extracted_signature", true);
      do_check_false(noMAR.exists());
      if (extractedSig.exists()) {
        extractedSig.remove(false);
      }
      extractMARSignature(noMAR, 0, extractedSig, wantFailure);
      do_check_false(extractedSig.exists());
    },
    
    cleanup_per_test: function() {
    }
  };

  cleanup();

  
  do_check_eq(run_tests(tests), Object.keys(tests).length - 1);

  do_register_cleanup(cleanup);
}
