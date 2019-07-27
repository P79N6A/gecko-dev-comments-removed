"use strict";

waitForExplicitFinish();

let pageSource =
  '<html><meta charset=UTF-8><body>' +
    '<img id="testImg" src="' + TESTROOT + 'big.png">' +
  '</body></html>';

let oldDiscardingPref, oldTab, newTab;
let prefBranch = Cc["@mozilla.org/preferences-service;1"]
                   .getService(Ci.nsIPrefService)
                   .getBranch('image.mem.');

function imgDiscardingFrameScript() {
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  function ImageDiscardObserver(result) {
    this.discard = function onDiscard(request) {
      result.wasDiscarded = true;
    }
  }

  function currentRequest() {
    let img = content.document.getElementById('testImg');
    img.QueryInterface(Ci.nsIImageLoadingContent);
    return img.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
  }

  function attachDiscardObserver(result) {
    
    let observer = new ImageDiscardObserver(result);
    let scriptedObserver = Cc["@mozilla.org/image/tools;1"]
                             .getService(Ci.imgITools)
                             .createScriptedObserver(observer);

    
    let request = currentRequest();
    return [ request.clone(scriptedObserver), scriptedObserver ];
  }

  
  var result = { wasDiscarded: false };
  var scriptedObserver;
  var clonedRequest;

  addMessageListener("test666317:testPart1", function(message) {
    
    let doc = content.document;
    let img = doc.getElementById('testImg');
    let canvas = doc.createElement('canvas');
    let ctx = canvas.getContext('2d');
    ctx.drawImage(img, 0, 0);

    
    
    
    
    [ clonedRequest, scriptedObserver ] = attachDiscardObserver(result)
    let decoded = clonedRequest.imageStatus & Ci.imgIRequest.STATUS_FRAME_COMPLETE ? true : false;

    message.target.sendAsyncMessage("test666317:testPart1:Answer",
                                    { decoded });
  });

  addMessageListener("test666317:wasImgDiscarded", function(message) {
    let discarded = result.wasDiscarded;

    
    clonedRequest.cancelAndForgetObserver(0);
    scriptedObserver = null;
    message.target.sendAsyncMessage("test666317:wasImgDiscarded:Answer",
                                    { discarded });
  });
}

function test() {
  
  oldDiscardingPref = prefBranch.getBoolPref('discardable');
  prefBranch.setBoolPref('discardable', true);

  
  oldTab = gBrowser.selectedTab;
  newTab = gBrowser.addTab('data:text/html,' + pageSource);
  gBrowser.selectedTab = newTab;

  
  gBrowser.getBrowserForTab(newTab)
          .addEventListener("pageshow", step2 );
}

function step2() {
  let mm = gBrowser.getBrowserForTab(newTab).QueryInterface(Ci.nsIFrameLoaderOwner)
           .frameLoader.messageManager;
  mm.loadFrameScript("data:,(" + escape(imgDiscardingFrameScript.toString()) + ")();", false);

  
  mm.addMessageListener("test666317:testPart1:Answer", function(message) {
    let decoded = message.data.decoded;
    ok(decoded, 'Image should initially be decoded.');

    
    
    gBrowser.selectedTab = oldTab;
    waitForFocus(() => {
      var os = Cc["@mozilla.org/observer-service;1"]
                 .getService(Ci.nsIObserverService);

      os.notifyObservers(null, 'memory-pressure', 'heap-minimize');

      
      mm.sendAsyncMessage("test666317:wasImgDiscarded");
    }, oldTab.contentWindow);
  });

  mm.addMessageListener("test666317:wasImgDiscarded:Answer", function(message) {
    let discarded = message.data.discarded;
    ok(discarded, 'Image should be discarded.');

    
    gBrowser.removeTab(newTab);
    prefBranch.setBoolPref('discardable', oldDiscardingPref);
    finish();
  });

  mm.sendAsyncMessage("test666317:testPart1");
}
