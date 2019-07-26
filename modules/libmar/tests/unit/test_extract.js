


function run_test() {

  





  function run_one_test(marFileName, files) { 
    
    let mar = do_get_file("data/" + marFileName);

    
    let outDir = do_get_file("out", true);
    do_check_false(outDir.exists());
    outDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);

    
    let outFiles = [];
    let refFiles = [];
    for (let i = 0; i < files.length; i++) {
      let outFile = do_get_file("out/" + files[i], true);
      do_check_false(outFile.exists());

      outFiles.push(outFile);
      refFiles.push(do_get_file("data/" + files[i]));
    }

    
    extractMAR(mar, outDir);

    
    for (let i = 0; i < files.length; i++) {
      do_check_true(outFiles[i].exists());
      let refFileData = getBinaryFileData(refFiles[i]);
      let outFileData = getBinaryFileData(outFiles[i]);
      compareBinaryData(refFileData, outFileData);
    }
  }

  
  let tests = {
    
    test_zero_sized: function() {
      return run_one_test("0_sized_mar.mar", ["0_sized_file"]);
    }, 
    
    test_one_byte: function() {
      return run_one_test("1_byte_mar.mar", ["1_byte_file"]);
    },
    
    test_binary_data: function() {
      return run_one_test("binary_data_mar.mar", ["binary_data_file"]);
    },
    
    
    test_no_pib: function() {
      return run_one_test("no_pib_mar.mar", ["binary_data_file"]);
    },
    
    
    test_no_pib_signed: function() {
      return run_one_test("signed_no_pib_mar.mar", ["binary_data_file"]);
    },
    
    
    test_pib_signed: function() {
      return run_one_test("signed_pib_mar.mar", ["binary_data_file"]);
    },
    
    test_multiple_file: function() {
      return run_one_test("multiple_file_mar.mar", 
                          ["0_sized_file", "1_byte_file", "binary_data_file"]);
    }, 
    
    
    cleanup_per_test: function() {
      let outDir = do_get_file("out", true);
      if (outDir.exists()) {
        outDir.remove(true);
      }
    }
  };

  
  do_check_eq(run_tests(tests), Object.keys(tests).length - 1);
}
