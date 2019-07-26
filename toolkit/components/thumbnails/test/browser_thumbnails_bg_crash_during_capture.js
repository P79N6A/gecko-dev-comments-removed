


function runTests() {
  let crashObserver = bgAddCrashObserver();

  
  let goodUrl = bgTestPageURL();
  yield bgCapture(goodUrl);
  ok(thumbnailExists(goodUrl), "Thumbnail should be cached after capture");
  removeThumbnail(goodUrl);

  
  let mm = bgInjectCrashContentScript();

  
  
  let waitUrl = bgTestPageURL({ wait: 30000 });
  let sawWaitUrlCapture = false;
  bgCapture(waitUrl, { onDone: () => {
    sawWaitUrlCapture = true;
    ok(!thumbnailExists(waitUrl), "Thumbnail should not have been saved due to the crash");
  }});
  bgCapture(goodUrl, { onDone: () => {
    ok(sawWaitUrlCapture, "waitUrl capture should have finished first");
    ok(thumbnailExists(goodUrl), "We should have recovered and completed the 2nd capture after the crash");
    removeThumbnail(goodUrl);
    
    ok(crashObserver.crashed, "Saw a crash from this test");
    next();
  }});

  info("Crashing the thumbnail content process.");
  mm.sendAsyncMessage("thumbnails-test:crash");
  yield true;
}
