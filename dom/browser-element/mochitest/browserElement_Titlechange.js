



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe1 = document.createElement('iframe');
  SpecialPowers.wrap(iframe1).mozbrowser = true;
  document.body.appendChild(iframe1);

  
  
  
  var iframe2 = document.createElement('iframe');
  SpecialPowers.wrap(iframe2).mozbrowser = true;
  document.body.appendChild(iframe2);

  
  
  var iframe3 = document.createElement('iframe');
  document.body.appendChild(iframe3);

  var numTitleChanges = 0;

  iframe1.addEventListener('mozbrowsertitlechange', function(e) {
    
    if (e.detail == '')
      return;

    numTitleChanges++;

    if (numTitleChanges == 1) {
      is(e.detail, 'Title');
      SpecialPowers.getBrowserFrameMessageManager(iframe1)
                   .loadFrameScript("data:,content.document.title='New title';",
                                     false);
      SpecialPowers.getBrowserFrameMessageManager(iframe2)
                   .loadFrameScript("data:,content.document.title='BAD TITLE 2';",
                                     false);
    }
    else if (numTitleChanges == 2) {
      is(e.detail, 'New title');
      iframe1.src = 'data:text/html,<html><head><title>Title 3</title></head><body></body></html>';
    }
    else if (numTitleChanges == 3) {
      is(e.detail, 'Title 3');
      SimpleTest.finish();
    }
    else {
      ok(false, 'Too many titlechange events.');
    }
  });

  iframe3.addEventListener('mozbrowsertitlechange', function(e) {
    ok(false, 'Should not get a titlechange event for iframe3.');
  });

  iframe1.src = 'data:text/html,<html><head><title>Title</title></head><body></body></html>';
  iframe2.src = 'data:text/html,<html><head><title>BAD TITLE</title></head><body></body></html>';
  iframe3.src = 'data:text/html,<html><head><title>SHOULD NOT GET EVENT</title></head><body></body></html>';
}

addEventListener('testready', runTest);
