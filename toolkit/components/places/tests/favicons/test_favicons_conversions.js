











let isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);



















function checkFaviconDataConversion(aFileName, aFileMimeType, aFileLength,
                                    aExpectConversion, aVaryOnWindows,
                                    aCallback) {
  let pageURI = NetUtil.newURI("http://places.test/page/" + aFileName);
  PlacesTestUtils.addVisits({ uri: pageURI, transition: TRANSITION_TYPED }).then(
    function () {
      let faviconURI = NetUtil.newURI("http://places.test/icon/" + aFileName);
      let fileData = readFileOfLength(aFileName, aFileLength);

      PlacesUtils.favicons.replaceFaviconData(faviconURI, fileData, fileData.length,
                                              aFileMimeType);
      PlacesUtils.favicons.setAndFetchFaviconForPage(pageURI, faviconURI, true,
        PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE,
        function CFDC_verify(aURI, aDataLen, aData, aMimeType) {
          if (!aExpectConversion) {
            do_check_true(compareArrays(aData, fileData));
            do_check_eq(aMimeType, aFileMimeType);
          } else {
            if (!aVaryOnWindows || !isWindows) {
              let expectedFile = do_get_file("expected-" + aFileName + ".png");
              do_check_true(compareArrays(aData, readFileData(expectedFile)));
            }
            do_check_eq(aMimeType, "image/png");
          }

          aCallback();
        });
    });
}




function run_test() {
  run_next_test();
}

add_test(function test_storing_a_normal_16x16_icon() {
  
  checkFaviconDataConversion("favicon-normal16.png", "image/png", 286,
                             false, false, run_next_test);
});

add_test(function test_storing_a_normal_32x32_icon() {
  
  checkFaviconDataConversion("favicon-normal32.png", "image/png", 344,
                             false, false, run_next_test);
});

add_test(function test_storing_an_oversize_16x16_icon() {
  
  
  checkFaviconDataConversion("favicon-big16.ico", "image/x-icon", 1406,
                             true, false, run_next_test);
});

add_test(function test_storing_an_oversize_4x4_icon() {
  
  
  checkFaviconDataConversion("favicon-big4.jpg", "image/jpeg", 4751,
                             true, false, run_next_test);
});

add_test(function test_storing_an_oversize_32x32_icon() {
  
  
  checkFaviconDataConversion("favicon-big32.jpg", "image/jpeg", 3494,
                             true, true, run_next_test);
});

add_test(function test_storing_an_oversize_48x48_icon() {
  
  
  
  
  checkFaviconDataConversion("favicon-big48.ico", "image/x-icon", 56646,
                             true, false, run_next_test);
});

add_test(function test_storing_an_oversize_64x64_icon() {
  
  
  checkFaviconDataConversion("favicon-big64.png", "image/png", 10698,
                             true, false, run_next_test);
});

add_test(function test_scaling_an_oversize_160x3_icon() {
  
  
  checkFaviconDataConversion("favicon-scale160x3.jpg", "image/jpeg", 5095,
                             true, false, run_next_test);
});

add_test(function test_scaling_an_oversize_3x160_icon() {
  
  
  checkFaviconDataConversion("favicon-scale3x160.jpg", "image/jpeg", 5059,
                             true, false, run_next_test);
});
