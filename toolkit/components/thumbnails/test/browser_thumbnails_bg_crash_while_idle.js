


function runTests() {
  let crashObserver = bgAddCrashObserver();

  
  let goodUrl = bgTestPageURL();
  yield bgCapture(goodUrl);
  ok(thumbnailExists(goodUrl), "Thumbnail should be cached after capture");
  removeThumbnail(goodUrl);

  
  let mm = bgInjectCrashContentScript();

  
  
  
  Services.obs.addObserver(function onCrash() {
    Services.obs.removeObserver(onCrash, "oop-frameloader-crashed");
    
    executeSoon(function() {
      
      bgCapture(goodUrl, { onDone: () => {
        ok(thumbnailExists(goodUrl), "We should have recovered and handled new capture requests");
        removeThumbnail(goodUrl);
        
        ok(crashObserver.crashed, "Saw a crash from this test");
        next();
      }});
    });
  } , "oop-frameloader-crashed", false);

  
  info("Crashing the thumbnail content process.");
  mm.sendAsyncMessage("thumbnails-test:crash");
  yield true;
}
