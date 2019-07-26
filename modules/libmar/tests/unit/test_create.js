


function run_test() {

  






  function run_one_test(refMARFileName, files, checkNoMAR) {
    if (checkNoMAR === undefined) {
      checkNoMAR = true;
    }

    
    let outMAR = do_get_file("out.mar", true);
    if (checkNoMAR) {
      do_check_false(outMAR.exists());
    }

    
    createMAR(outMAR, do_get_file("data"), files);

    
    let refMAR = do_get_file("data/" + refMARFileName);
    let refMARData = getBinaryFileData(refMAR);

    
    let outMARData = getBinaryFileData(outMAR);
    compareBinaryData(outMARData, refMARData);
  }

  
  let tests = {
    
    test_zero_sized: function() {
      return run_one_test(refMARPrefix + "0_sized_mar.mar", ["0_sized_file"]);
    },
    
    test_one_byte: function() {
      return run_one_test(refMARPrefix + "1_byte_mar.mar", ["1_byte_file"]);
    },
    
    test_binary_data: function() {
      return run_one_test(refMARPrefix + "binary_data_mar.mar", 
                          ["binary_data_file"]);
    },
    
    test_multiple_file: function() {
      return run_one_test(refMARPrefix + "multiple_file_mar.mar", 
                          ["0_sized_file", "1_byte_file", "binary_data_file"]);
    },
    
    
    test_overwrite_already_exists: function() {
      let differentFile = do_get_file("data/1_byte_mar.mar");
      let outMARDir = do_get_file(".");
      differentFile.copyTo(outMARDir, "out.mar");
      return run_one_test(refMARPrefix + "binary_data_mar.mar", 
                          ["binary_data_file"], false);
    },
    
    cleanup_per_test: function() {
      let outMAR = do_get_file("out.mar", true);
      if (outMAR.exists()) {
        outMAR.remove(false);
      }
    }
  };

  
  do_check_eq(run_tests(tests), Object.keys(tests).length - 1);
}
