





var gSmallTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.233 },
  { name:"small-shot.ogg", type:"audio/ogg", duration:0.276 },
  { name:"seek.webm", type:"video/webm", duration:3.966 },
  { name:"bogus.duh", type:"bogus/duh" }
];



var gProgressTests = [
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0, size:11069 },
  { name:"big.wav", type:"audio/x-wav", duration:9.0, size:102444 },
  { name:"seek.ogv", type:"video/ogg", duration:3.966, size:285310 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.233, size:28942 },
  { name:"seek.webm", type:"video/webm", duration:3.966, size:215529 },
  { name:"bogus.duh", type:"bogus/duh" }
];



var gCloneTests = gSmallTests.concat([
  
  { name:"bug520908.ogv", type:"video/ogg", duration:9000 },
]);



var gReplayTests = gSmallTests.concat([
  { name:"bug533822.ogg", type:"audio/ogg" },
]);



var gPausedAfterEndedTests = gSmallTests.concat([
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"small-shot.ogg", type:"video/ogg", duration:0.276 }
]);





var gPlayTests = [
  
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0 },
  
  { name:"r11025_u8_c1_trunc.wav", type:"audio/x-wav", duration:1.8 },
  
  { name:"r11025_s16_c1_trailing.wav", type:"audio/x-wav", duration:1.0 },
  
  { name:"r16000_u8_c1_list.wav", type:"audio/x-wav", duration:4.2 },

  
  { name:"bug461281.ogg", type:"application/ogg", duration:2.208 },

  
  { name:"bug482461.ogv", type:"video/ogg", duration:4.34 },
  
  { name:"bug482461-theora.ogv", type:"video/ogg", duration:4.138 },
  
  { name:"bug500311.ogv", type:"video/ogg", duration:1.96 },
  
  { name:"small-shot.ogg", type:"video/ogg", duration:0.276 },
  
  { name:"short-video.ogv", type:"video/ogg", duration:1.081 },
  
  { name:"bug504613.ogv", type:"video/ogg", duration:Number.NaN },
  
  { name:"bug516323.ogv", type:"video/ogg", duration:4.208 },
  
  { name:"bug556821.ogv", type:"video/ogg", duration:2.551 },

  
  { name:"beta-phrasebook.ogg", type:"audio/ogg", duration:4.01 },
  
  { name:"bug520493.ogg", type:"audio/ogg", duration:0.458 },
  
  { name:"bug520500.ogg", type:"audio/ogg", duration:0.123 },

  
  { name:"bug499519.ogv", type:"video/ogg", duration:0.24 },
  { name:"bug506094.ogv", type:"video/ogg", duration:0 },
  { name:"bug498855-1.ogv", type:"video/ogg", duration:0.24 },
  { name:"bug498855-2.ogv", type:"video/ogg", duration:0.24 },
  { name:"bug498855-3.ogv", type:"video/ogg", duration:0.24 },
  { name:"bug504644.ogv", type:"video/ogg", duration:1.6 },
  { name:"chain.ogv", type:"video/ogg", duration:Number.NaN },
  { name:"bug523816.ogv", type:"video/ogg", duration:0.533 },
  { name:"bug495129.ogv", type:"video/ogg", duration:2.41 },
  
  { name:"bug498380.ogv", type:"video/ogg", duration:0.533 },
  { name:"bug495794.ogg", type:"audio/ogg", duration:0.3 },
  { name:"bug557094.ogv", type:"video/ogg", duration:0.24 },
  { name:"audio-overhang.ogg", type:"audio/ogg", duration:2.3 },
  { name:"video-overhang.ogg", type:"audio/ogg", duration:3.966 },

  
  { name:"audio-gaps.ogg", type:"audio/ogg", duration:2.208 },

  
  { name:"redirect.sjs?domain=mochi.test:8888&file=320x240.ogv",
    type:"video/ogg", duration:0.233 },

  
  { name:"seek.webm", type:"video/webm", duration:3.966 },

  
  { name:"split.webm", type:"video/webm", duration:1.967 },
  
  
  { name:"seek.yuv", type:"video/x-raw-yuv", duration:1.833 },
  
  
  
  
  { name:"spacestorm-1000Hz-100ms.ogg", type:"audio/ogg", duration:0.099 },

  { name:"bogus.duh", type:"bogus/duh", duration:Number.NaN }
  
];




function fileUriToSrc(path, mustExist) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  const Ci = Components.interfaces;
  const Cc = Components.classes;
  const Cr = Components.results;
  var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  var f = dirSvc.get("CurWorkD", Ci.nsILocalFile);
  var split = path.split("/");
  for(var i = 0; i < split.length; ++i) {
    f.append(split[i]);
  }
  if (mustExist && !f.exists()) {
    ok(false, "We expected '" + path + "' to exist, but it doesn't!");
  }
  return f.path;
}




var gInfoLeakTests = [
  {
    type: 'video/ogg',
    src: fileUriToSrc("tests/content/media/test/320x240.ogv", true),
  },{
    type: 'video/ogg',
    src: fileUriToSrc("tests/content/media/test/404.ogv", false),
  }, {
    type: 'audio/x-wav',
    src: fileUriToSrc("tests/content/media/test/r11025_s16_c1.wav", true),
  }, {
    type: 'audio/x-wav',
    src: fileUriToSrc("tests/content/media/test/404.wav", false),
  }, {
    type: 'audio/ogg',
    src: fileUriToSrc("tests/content/media/test/bug461281.ogg", true),
  }, {
    type: 'audio/ogg',
    src: fileUriToSrc("tests/content/media/test/404.ogg", false),
  }, {
    type: 'video/webm',
    src: fileUriToSrc("tests/content/media/test/seek.webm", true),
  }, {
    type: 'video/webm',
    src: fileUriToSrc("tests/content/media/test/404.webm", false),
  }, {
    type: 'video/ogg',
    src: 'http://localhost/404.ogv',
  }, {
    type: 'audio/x-wav',
    src: 'http://localhost/404.wav',
  }, {
    type: 'video/webm',
    src: 'http://localhost/404.webm',
  }, {
    type: 'video/ogg',
    src: 'http://example.com/tests/content/media/test/test_info_leak.html'
  }, {
    type: 'audio/ogg',
    src: 'http://example.com/tests/content/media/test/test_info_leak.html'
  }
];







var gErrorTests = [
  { name:"bogus.wav", type:"audio/x-wav" },
  { name:"bogus.ogv", type:"video/ogg" },
  { name:"448636.ogv", type:"video/ogg" },
  { name:"bug504843.ogv", type:"video/ogg" },
  { name:"bug501279.ogg", type:"audio/ogg" },
  { name:"bug580982.webm", type:"video/webm" },
  { name:"bug603918.webm", type:"video/webm" },
  { name:"bug604067.webm", type:"video/webm" },
  { name:"bogus.duh", type:"bogus/duh" }
];


var gSeekTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"audio.wav", type:"audio/x-wav", duration:0.031247 },
  { name:"seek.ogv", type:"video/ogg", duration:3.966 },
  { name:"320x240.ogv", type:"video/ogg", duration:0.233 },
  { name:"seek.webm", type:"video/webm", duration:3.966 },
  { name:"bug516323.indexed.ogv", type:"video/ogg", duration:4.208 },
  { name:"split.webm", type:"video/webm", duration:1.967 },
  { name:"bogus.duh", type:"bogus/duh", duration:123 }
];


var gAudioTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"sound.ogg", type:"audio/ogg" },
  { name:"bogus.duh", type:"bogus/duh", duration:123 }
];



var g404Tests = [
  { name:"404.wav", type:"audio/x-wav" },
  { name:"404.ogv", type:"video/ogg" },
  { name:"404.oga", type:"audio/ogg" },
  { name:"404.webm", type:"video/webm" },
  { name:"bogus.duh", type:"bogus/duh" }
];




var gDecodeErrorTests = [
  
  { name:"r11025_msadpcm_c1.wav", type:"audio/x-wav" },
  { name:"dirac.ogg", type:"video/ogg" },
  
  { name:"bogus.wav", type:"audio/x-wav" },
  { name:"bogus.ogv", type:"video/ogg" },

  { name:"bogus.duh", type:"bogus/duh" }
];

function checkMetadata(msg, e, test) {
  if (test.width) {
    is(e.videoWidth, test.width, msg + " video width");
  }
  if (test.height) {
    is(e.videoHeight, test.height, msg + " video height");
  }
  if (test.duration) {
    ok(Math.abs(e.duration - test.duration) < 0.1,
       msg + " duration (" + e.duration + ") should be around " + test.duration);
  }
}



function getPlayableVideo(candidates) {
  var v = document.createElement("video");
  var resources = candidates.filter(function(x){return /^video/.test(x.type) && v.canPlayType(x.type);});
  if (resources.length > 0)
    return resources[0];
  return null;
}




var PARALLEL_TESTS = 2;



const DEBUG_TEST_LOOP_FOREVER = false;
















function MediaTestManager() {

  
  
  
  
  
  
  
  
  this.runTests = function(tests, startTest) {
    this.testNum = 0;
    this.tests = tests;
    this.startTest = startTest;
    this.tokens = [];
    
    SimpleTest.waitForExplicitFinish();
    this.nextTest();
  }
  
  
  
  this.started = function(token) {
    this.tokens.push(token);
  }
  
  
  
  
  
  this.finished = function(token) {
    var i = this.tokens.indexOf(token);
    if (i != -1) {
      
      this.tokens.splice(i, 1);
    }
    if (this.tokens.length == 0) {
      this.nextTest();
    }
  }
  
  
  
  this.nextTest = function() {
    
    
    
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    Components.utils.forceGC();
    if (this.testNum == this.tests.length && !DEBUG_TEST_LOOP_FOREVER) {
      if (this.onFinished) {
        this.onFinished();
      }
      mediaTestCleanup();
      SimpleTest.finish();
      return;
    }
    while (this.testNum < this.tests.length && this.tokens.length < PARALLEL_TESTS) {
      var test = this.tests[this.testNum];
      var token = (test.name ? (test.name + "-"): "") + this.testNum;
      this.testNum++;

      if (DEBUG_TEST_LOOP_FOREVER && this.testNum == this.tests.length) {
        this.testNum = 0;
      }
      
      
      if (test.type && !document.createElement('video').canPlayType(test.type))
        continue;
      
      
      this.startTest(test, token);
      
    }
    if (this.tokens.length == 0) {
      
      SimpleTest.finish();
    }
  }
}




function mediaTestCleanup() {
    var V = document.getElementsByTagName("video");
    for (i=0; i<V.length; i++) {
      V[i].parentNode.removeChild(V[i]);
      V[i] = null;
    }
    var A = document.getElementsByTagName("audio");
    for (i=0; i<A.length; i++) {
      A[i].parentNode.removeChild(A[i]);
      A[i] = null;
    }
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    Components.utils.forceGC();
}

(function() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                               .getService(Components.interfaces.nsIPrefService);
  var branch = prefService.getBranch("media.");
  var oldDefault = 2;
  var oldAuto = 3;
  try {
    oldDefault = branch.getIntPref("preload.default");
    oldAuto    = branch.getIntPref("preload.auto");
  } catch(ex) { }
  branch.setIntPref("preload.default", 2); 
  branch.setIntPref("preload.auto", 3); 

  window.addEventListener("unload", function() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    branch.setIntPref("preload.default", oldDefault);
    branch.setIntPref("preload.auto", oldAuto);
  }, false);
 })();
