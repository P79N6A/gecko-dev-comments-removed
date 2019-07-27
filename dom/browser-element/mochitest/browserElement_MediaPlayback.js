



'use strict';

const { Services } = SpecialPowers.Cu.import('resource://gre/modules/Services.jsm');

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();




function playMediaScript() {
  var audio = new content.Audio();
  content.document.body.appendChild(audio);
  audio.oncanplay = function() {
    audio.play();
  };
  audio.src = 'audio.ogg';
}




function createFrame() {
  let iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');
  document.body.appendChild(iframe);
  return iframe;
}

function runTest() {
  SimpleTest.waitForExplicitFinish();

  let iframe = createFrame();
  let iframe2 = createFrame();

  
  
  iframe.addEventListener('mozbrowserloadend', () => {
    let mm = SpecialPowers.getBrowserFrameMessageManager(iframe);
    mm.loadFrameScript('data:,(' + playMediaScript.toString() + ')();', false);
  });

  
  
  
  let expectedNextData = 'active';
  iframe.addEventListener('mozbrowsermediaplaybackchange', (e) => {
    is(e.detail, expectedNextData, 'Audio detail should be correct')
    is(e.target, iframe, 'event target should be the first iframe')
    if (e.detail === 'inactive') {
      SimpleTest.finish();
    }
    expectedNextData = 'inactive';
  });

  
  iframe2.addEventListener('mozbrowsermediaplaybackchange', (e) => {
    ok(false,
       'mozbrowsermediaplaybackchange should dispatch to the correct browser');
  });

  
  iframe.src = browserElementTestHelpers.emptyPage1;
}

addEventListener('testready', () => {
  
  SpecialPowers.pushPrefEnv({"set": [["media.useAudioChannelService", true]]},
                            runTest);
});
