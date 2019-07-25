waitForExplicitFinish();
requestLongerTimeout(2); 



var gTimer;



function testBFCache() {
  function theTest() {
    var abort = false;
    var chances, gImage, gFrames;
    gBrowser.selectedTab = gBrowser.addTab(TESTROOT + "image.html");
    gBrowser.selectedBrowser.addEventListener("pageshow", function () {
      gBrowser.selectedBrowser.removeEventListener("pageshow", arguments.callee, true);
      var window = gBrowser.contentWindow;
      
      
      if (!actOnMozImage(window.document, "img1", function(image) {
        gImage = image;
        gFrames = gImage.framesNotified;
      })) {
        gBrowser.removeCurrentTab();
        abort = true;
      }
      goer.next();
    }, true);
    yield;
    if (abort) {
      finish();
      yield; 
    }

    
    chances = 120;
    do {
      gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      gTimer.initWithCallback(function() {
        if (gImage.framesNotified >= 20) {
          goer.send(true);
        } else {
          chances--;
          goer.send(chances == 0); 
        }
      }, 500, Ci.nsITimer.TYPE_ONE_SHOT);
    } while (!(yield));
    is(chances > 0, true, "Must have animated a few frames so far");

    
    gBrowser.loadURI("about:blank");

    
    
    gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    gTimer.initWithCallback(function() {
      gFrames = gImage.framesNotified;
      gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      gTimer.initWithCallback(function() {
        
        is(gImage.framesNotified == gFrames, true, "Must have not animated in bfcache!");
        goer.next();
      }, 4000, Ci.nsITimer.TYPE_ONE_SHOT); 
    }, 0, Ci.nsITimer.TYPE_ONE_SHOT); 
    yield;

    
    gBrowser.goBack();

    chances = 120;
    do {
      gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      gTimer.initWithCallback(function() {
        if (gImage.framesNotified - gFrames >= 20) {
          goer.send(true);
        } else {
          chances--;
          goer.send(chances == 0); 
        }
      }, 500, Ci.nsITimer.TYPE_ONE_SHOT);
    } while (!(yield));
    is(chances > 0, true, "Must have animated once out of bfcache!");

    
    
    
    
    
    var doc = gBrowser.selectedBrowser.contentWindow.document;
    var div = doc.getElementById("background_div");
    div.innerHTML += '<img src="animated2.gif" id="img3">';
    actOnMozImage(doc, "img3", function(image) {
      is(Math.abs(image.framesNotified - gImage.framesNotified)/gImage.framesNotified < 0.5, true,
         "Must have also animated the background image, and essentially the same # of frames");
    });

    gBrowser.removeCurrentTab();

    nextTest();
  }

  var goer = theTest();
  goer.next();
}



function testSharedContainers() {
  function theTest() {
    var gImages = [];
    var gFrames;

    gBrowser.selectedTab = gBrowser.addTab(TESTROOT + "image.html");
    gBrowser.selectedBrowser.addEventListener("pageshow", function () {
      gBrowser.selectedBrowser.removeEventListener("pageshow", arguments.callee, true);
      actOnMozImage(gBrowser.contentDocument, "img1", function(image) {
        gImages[0] = image;
        gFrames = image.framesNotified; 
                                        
      });
      goer.next();
    }, true);
    yield;

    
    gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    gTimer.initWithCallback(function() {
      goer.next();
    }, 1500, Ci.nsITimer.TYPE_ONE_SHOT);
    yield;

    gBrowser.selectedTab = gBrowser.addTab(TESTROOT + "imageX2.html");
    gBrowser.selectedBrowser.addEventListener("pageshow", function () {
      gBrowser.selectedBrowser.removeEventListener("pageshow", arguments.callee, true);
      [1,2].forEach(function(i) {
        actOnMozImage(gBrowser.contentDocument, "img"+i, function(image) {
          gImages[i] = image;
        });
      });
      goer.next();
    }, true);
    yield;

    chances = 120;
    do {
      gTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      gTimer.initWithCallback(function() {
        if (gImages[0].framesNotified - gFrames >= 10) {
          goer.send(true);
        } else {
          chances--;
          goer.send(chances == 0); 
        }
      }, 500, Ci.nsITimer.TYPE_ONE_SHOT);
    } while (!(yield));
    is(chances > 0, true, "Must have been animating while showing several images");

    
    var theFrames = null;
    [0,1,2].forEach(function(i) {
      var frames = gImages[i].framesNotified;
      if (theFrames == null) {
        theFrames = frames;
      } else {
        is(theFrames, frames, "Sharing the same imgContainer means *exactly* the same frame counts!");
      }
    });

    gBrowser.removeCurrentTab();
    gBrowser.removeCurrentTab();

    nextTest();
  }

  var goer = theTest();
  goer.next();
}

var tests = [testBFCache, testSharedContainers];

function nextTest() {
  if (tests.length == 0) {
    finish();
    return;
  }
  tests.shift()();
}

function test() {
  nextTest();
}

