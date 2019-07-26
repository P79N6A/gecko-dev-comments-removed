





var gSmallTests = [
  { name:"small-shot.ogg", type:"audio/ogg", duration:0.276 },
  { name:"small-shot.m4a", type:"audio/mp4", duration:0.29 },
  { name:"small-shot.mp3", type:"audio/mpeg", duration:0.27 },
  { name:"small-shot-mp3.mp4", type:"audio/mp4; codecs=mp3", duration:0.34 },
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.266 },
  { name:"seek.webm", type:"video/webm", width:320, height:240, duration:3.966 },
  { name:"vp9.webm", type:"video/webm", width:320, height:240, duration:4 },
  { name:"detodos.opus", type:"audio/ogg; codecs=opus", duration:2.9135 },
  { name:"gizmo.mp4", type:"video/mp4", duration:5.56 },
  { name:"bogus.duh", type:"bogus/duh" }
];

if (SpecialPowers.Services.appinfo.name != "B2G") {
  
  
  
  

  gSmallTests = gSmallTests.concat([
    { name:"sample.3gp", type:"video/3gpp", duration:4.933 },
    { name:"sample.3g2", type:"video/3gpp2", duration:4.933 }
  ]);
}


var gVideoTests = [
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.266 },
  { name:"seek.webm", type:"video/webm", width:320, height:240, duration:3.966 },
  { name:"bogus.duh", type:"bogus/duh" }
];



var gProgressTests = [
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0, size:11069 },
  { name:"big.wav", type:"audio/x-wav", duration:9.278981, size:102444 },
  { name:"seek.ogv", type:"video/ogg", duration:3.966, size:285310 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.266, size:28942 },
  { name:"seek.webm", type:"video/webm", duration:3.966, size:215529 },
  { name:"gizmo.mp4", type:"video/mp4", duration:5.56, size:383631 },
  { name:"bogus.duh", type:"bogus/duh" }
];


var gPlayedTests = [
  { name:"big.wav", type:"audio/x-wav", duration:9.0 },
  { name:"seek.ogv", type:"video/ogg", duration:3.966 },
  { name:"seek.webm", type:"video/webm", duration:3.966 },
  { name:"gizmo.mp4", type:"video/mp4", duration:5.56 },
  { name:"owl.mp3", type:"audio/mpeg", duration:3.29 },
  { name:"vbr.mp3", type:"audio/mpeg", duration:10.0 },
  { name:"bug495794.ogg", type:"audio/ogg", duration:0.3 }
];



var cloneKey = Math.floor(Math.random()*100000000);
var gCloneTests = gSmallTests.concat([
  
  { name:"bug520908.ogv", type:"video/ogg", duration:9000 },
  
  { name:"dynamic_resource.sjs?key=" + cloneKey + "&res1=320x240.ogv&res2=short-video.ogv",
    type:"video/ogg", duration:0.266 },
]);



var gReplayTests = gSmallTests.concat([
  { name:"bug533822.ogg", type:"audio/ogg" },
]);



var gPausedAfterEndedTests = gSmallTests.concat([
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"small-shot.ogg", type:"video/ogg", duration:0.276 }
]);



var gTrackTests = [
  { name:"big.wav", type:"audio/x-wav", duration:9.278981, size:102444, hasAudio:true, hasVideo:false },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.266, size:28942, hasAudio:false, hasVideo:true },
  { name:"short-video.ogv", type:"video/ogg", duration:1.081, hasAudio:true, hasVideo:true },
  { name:"seek.webm", type:"video/webm", duration:3.966, size:215529, hasAudio:false, hasVideo:true },
  { name:"bogus.duh", type:"bogus/duh" }
];



var gMediaRecorderTests = [
  { name:"detodos.opus", type:"audio/ogg; codecs=opus", duration:2.9135 }
];





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
  { name:"multiple-bos.ogg", type:"video/ogg", duration:0.431 },
  { name:"audio-overhang.ogg", type:"audio/ogg", duration:2.3 },
  { name:"video-overhang.ogg", type:"audio/ogg", duration:3.966 },

  
  { name:"audio-gaps.ogg", type:"audio/ogg", duration:2.208 },

  
  { name:"redirect.sjs?domain=mochi.test:8888&file=320x240.ogv",
    type:"video/ogg", duration:0.266 },

  
  { name:"seek.webm", type:"video/webm", duration:3.966 },

  
  { name:"split.webm", type:"video/webm", duration:1.967 },

  
  
  { name:"vp9cake.webm", type:"video/webm", duration:7.966 },

  
  { name:"seek.yuv", type:"video/x-raw-yuv", duration:1.833 },

  
  
  
  { name:"spacestorm-1000Hz-100ms.ogg", type:"audio/ogg", duration:0.099 },

  
  { name:"detodos.opus", type:"audio/ogg; codecs=opus", duration:2.9135 },
  
  { name:"detodos.webm", type:"audio/webm; codecs=opus", duration:2.9135 },

  
  { name:"test-1-mono.opus", type:"audio/ogg; codecs=opus", duration:1.044 },
  { name:"test-2-stereo.opus", type:"audio/ogg; codecs=opus", duration:2.925 },
  { name:"test-3-LCR.opus", type:"audio/ogg; codecs=opus", duration:4.214 },
  { name:"test-4-quad.opus", type:"audio/ogg; codecs=opus", duration:6.234 },
  { name:"test-5-5.0.opus", type:"audio/ogg; codecs=opus", duration:7.558 },
  { name:"test-6-5.1.opus", type:"audio/ogg; codecs=opus", duration:10.333 },
  { name:"test-7-6.1.opus", type:"audio/ogg; codecs=opus", duration:11.690 },
  { name:"test-8-7.1.opus", type:"audio/ogg; codecs=opus", duration:13.478 },

  { name:"gizmo.mp4", type:"video/mp4", duration:5.56 },

  { name:"small-shot.m4a", type:"audio/mp4", duration:0.29 },
  { name:"small-shot.mp3", type:"audio/mpeg", duration:0.27 },
  { name:"owl.mp3", type:"audio/mpeg", duration:3.29 },
  
  
  { name:"owl-funny-id3.mp3", type:"audio/mpeg", duration:3.29 },
  
  
  { name:"owl-funnier-id3.mp3", type:"audio/mpeg", duration:3.29 },
  
  
  
  
  { name:"huge-id3.mp3", type:"audio/mpeg", duration:1.00 },
  
  
  
  
  { name:"vbr-head.mp3", type:"audio/mpeg", duration:10.00 },

  
  { name:"bogus.duh", type:"bogus/duh", duration:Number.NaN }
];


var gSnifferTests = [
  { name:"big.wav", type:"audio/x-wav", duration:9.278981, size:102444 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240, duration:0.233, size:28942 },
  { name:"seek.webm", type:"video/webm", duration:3.966, size:215529 },
  { name:"gizmo.mp4", type:"video/mp4", duration:5.56, size:383631 },
  
  { name:"id3tags.mp3", type:"audio/mpeg", duration:0.28, size:3530},
  { name:"bogus.duh", type:"bogus/duh" }
];


var gInvalidTests = [
  { name:"invalid-m0c0.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-m0c3.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-m1c0.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-m1c9.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-m2c0.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-m2c1.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-cmap-short.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-cmap-s0c0.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-cmap-s0c2.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-cmap-s1c2.opus", type:"audio/ogg; codecs=opus"},
  { name:"invalid-preskip.webm", type:"audio/webm; codecs=opus"},
];




function fileUriToSrc(path, mustExist) {
  
  if (navigator.appVersion.indexOf("Android") != -1 || SpecialPowers.Services.appinfo.name == "B2G")
    return path;

  const Ci = SpecialPowers.Ci;
  const Cc = SpecialPowers.Cc;
  const Cr = SpecialPowers.Cr;
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


function range_equals(r1, r2) {
  if (r1.length != r2.length) {
    return false;
  }
  for (var i = 0; i < r1.length; i++) {
    if (r1.start(i) != r2.start(i) || r1.end(i) != r2.end(i)) {
      return false;
    }
  }
  return true;
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
  { name:"320x240.ogv", type:"video/ogg", duration:0.266 },
  { name:"seek.webm", type:"video/webm", duration:3.966 },
  { name:"bug516323.indexed.ogv", type:"video/ogg", duration:4.208333 },
  { name:"split.webm", type:"video/webm", duration:1.967 },
  { name:"detodos.opus", type:"audio/ogg; codecs=opus", duration:2.9135 },
  { name:"gizmo.mp4", type:"video/mp4", duration:5.56 },
  { name:"owl.mp3", type:"audio/mpeg", duration:3.29 },
  { name:"bogus.duh", type:"bogus/duh", duration:123 }
];

var gFastSeekTests = [
  { name:"gizmo.mp4", type:"video/mp4", keyframes:[0, 1.0, 2.0, 3.0, 4.0, 5.0 ] },
  
  { name:"seek.webm", type:"video/webm", keyframes:[0, 0.8, 1.6, 2.4, 3.2]},
  
  
  
  { name:"bug516323.indexed.ogv", type:"video/ogg", keyframes:[0, 0.46, 3.06] },
];

function IsWindows8OrLater() {
  var re = /Windows NT (\d.\d)/;
  var winver = navigator.userAgent.match(re);
  return winver && winver.length == 2 && parseFloat(winver[1]) >= 6.2;
}



var gUnseekableTests = [
  { name:"no-cues.webm", type:"video/webm" },
  { name:"bogus.duh", type:"bogus/duh"}
];



if (navigator.userAgent.indexOf("Windows") != -1 && IsWindows8OrLater()) {
  gUnseekableTests = gUnseekableTests.concat([
    { name:"big-buck-bunny-unseekable.mp4", type:"video/mp4" }
  ]);
}

var androidVersion = SpecialPowers.Cc['@mozilla.org/system-info;1']
                                  .getService(SpecialPowers.Ci.nsIPropertyBag2)
                                  .getProperty('version');

if (navigator.userAgent.indexOf("Mobile") != -1 && androidVersion >= 18) {
  gUnseekableTests = gUnseekableTests.concat([
    { name:"street.mp4", type:"video/mp4" }
  ]);
}


var gAudioTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"sound.ogg", type:"audio/ogg" },
  { name:"owl.mp3", type:"audio/mpeg", duration:3.29 },
  { name:"small-shot.m4a", type:"audio/mp4", duration:0.29 },
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


var gFragmentTests = [
  { name:"big.wav", type:"audio/x-wav", duration:9.278981, size:102444 }
];



var gChainingTests = [
  
  
  { name:"chain.ogg", type:"audio/ogg", links: 4},
  { name:"chain.opus", type:"audio/ogg; codec=opus", links: 4},
  
  
  { name:"variable-channel.ogg", type:"audio/ogg", links: 1 },
  { name:"variable-channel.opus", type:"audio/ogg; codec=opus", links: 1 },
  
  
  { name:"variable-samplerate.ogg", type:"audio/ogg", links: 1 },
  
  
  
  { name:"variable-samplerate.opus", type:"audio/ogg; codec=opus", links: 2 },
  
  
  { name:"chained-video.ogv", type:"video/ogg", links: 1 },
  
  
  { name:"chained-audio-video.ogg", type:"video/ogg", links: 4 },
  
  
  { name:"variable-preskip.opus", type:"audio/ogg; codec=opus", links: 2 },
  { name:"bogus.duh", type:"bogus/duh" }
];




var gAspectRatioTests = [
  { name:"VID_0001.ogg", type:"video/ogg", duration:19.966 }
];



var gMetadataTests = [
  
  { name:"short-video.ogv", tags: {
      TITLE:"Lepidoptera",
      ARTIST:"Epoq",
      ALBUM:"Kahvi Collective",
      DATE:"2002",
      COMMENT:"http://www.kahvi.org",
    }
  },
  { name:"bug516323.ogv", tags: {
      GENRE:"Open Movie",
      ENCODER:"Audacity",
      TITLE:"Elephants Dream",
      ARTIST:"Silvia Pfeiffer",
      COMMENTS:"Audio Description"
    }
  },
  { name:"bug516323.indexed.ogv", tags: {
      GENRE:"Open Movie",
      ENCODER:"Audacity",
      TITLE:"Elephants Dream",
      ARTIST:"Silvia Pfeiffer",
      COMMENTS:"Audio Description"
    }
  },
  { name:"detodos.opus", tags: {
      title:"De todos. Para todos.",
      artist:"Mozilla.org"
    }
  },
  { name:"sound.ogg", tags: { } },
  { name:"small-shot.ogg", tags: {
      title:"Pew SFX"
    }
  },
  { name:"badtags.ogg", tags: {
      
      
      title:"Invalid comments test file",
      empty:"",
      "":"empty",
      "{- [(`!@\"#$%^&')] -}":"valid tag name, surprisingly"
      
      
      
      
      
      
    }
  },
  { name:"wave_metadata.wav", tags: {
      name:"Track Title",
      artist:"Artist Name",
      comments:"Comments",
    }
  },
  { name:"wave_metadata_utf8.wav", tags: {
      name:"歌曲名稱",
      artist:"作曲者",
      comments:"註解",
    }
  },
  { name:"wave_metadata_unknown_tag.wav", tags: {
      name:"Track Title",
      comments:"Comments",
    }
  },
  { name:"wave_metadata_bad_len.wav", tags: {
      name:"Track Title",
      artist:"Artist Name",
    }
  },
  { name:"wave_metadata_bad_no_null.wav", tags: {
      name:"Track Title",
      artist:"Artist Name",
      comments:"Comments!!",
    }
  },
  { name:"wave_metadata_bad_utf8.wav", tags: {
      name:"歌曲名稱",
      comments:"註解",
    }
  },
  { name:"wavedata_u8.wav", tags: { }
  },
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

function getPlayableAudio(candidates) {
  var v = document.createElement("audio");
  var resources = candidates.filter(function(x){return /^audio/.test(x.type) && v.canPlayType(x.type);});
  if (resources.length > 0)
    return resources[0];
  return null;
}


function getMajorMimeType(mimetype) {
  if (/^video/.test(mimetype)) {
    return "video";
  } else {
    return "audio";
  }
}

function removeNodeAndSource(n) {
  n.remove();
  
  n.src = "";
  while (n.firstChild) {
    n.removeChild(n.firstChild);
  }
}




var PARALLEL_TESTS = 2;



const DEBUG_TEST_LOOP_FOREVER = false;
















function MediaTestManager() {

  
  
  
  
  
  
  
  
  this.runTests = function(tests, startTest) {
    this.startTime = new Date();
    SimpleTest.info("Started " + this.startTime + " (" + this.startTime.getTime()/1000 + "s)");
    this.testNum = 0;
    this.tests = tests;
    this.startTest = startTest;
    this.tokens = [];
    this.isShutdown = false;
    this.numTestsRunning = 0;
    
    SimpleTest.waitForExplicitFinish();
    this.nextTest();
  }

  
  
  this.started = function(token) {
    this.tokens.push(token);
    this.numTestsRunning++;
    is(this.numTestsRunning, this.tokens.length, "[started " + token + "] Length of array should match number of running tests");
  }

  this.watchdog = null;

  this.watchdogFn = function() {
    if (this.tokens.length > 0) {
      info("Watchdog remaining tests= " + this.tokens);
    }
  }

  
  
  
  
  this.finished = function(token) {
    var i = this.tokens.indexOf(token);
    if (i != -1) {
      
      this.tokens.splice(i, 1);
    }

    if (this.watchdog) {
      clearTimeout(this.watchdog);
      this.watchdog = null;
    }

    info("[finished " + token + "] remaining= " + this.tokens);
    this.numTestsRunning--;
    is(this.numTestsRunning, this.tokens.length, "[finished " + token + "] Length of array should match number of running tests");
    if (this.tokens.length < PARALLEL_TESTS) {
      this.nextTest();
      this.watchdog = setTimeout(this.watchdogFn.bind(this), 10000);
    }
  }

  
  
  this.nextTest = function() {
    
    
    
    SpecialPowers.exactGC(window, function(){
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

      if (this.testNum == this.tests.length &&
          !DEBUG_TEST_LOOP_FOREVER &&
          this.tokens.length == 0 &&
          !this.isShutdown)
      {
        this.isShutdown = true;
        if (this.onFinished) {
          this.onFinished();
        }
        mediaTestCleanup();
        var end = new Date();
        SimpleTest.info("Finished at " + end + " (" + (end.getTime() / 1000) + "s)");
        SimpleTest.info("Running time: " + (end.getTime() - this.startTime.getTime())/1000 + "s");
        SimpleTest.finish();
        return;
      }
    }.bind(this));
  }
}




function mediaTestCleanup() {
    var V = document.getElementsByTagName("video");
    for (i=0; i<V.length; i++) {
      removeNodeAndSource(V[i]);
      V[i] = null;
    }
    var A = document.getElementsByTagName("audio");
    for (i=0; i<A.length; i++) {
      removeNodeAndSource(A[i]);
      A[i] = null;
    }
    SpecialPowers.forceGC();
}

(function() {
  
  var prefService = SpecialPowers.wrap(SpecialPowers.Components)
                                 .classes["@mozilla.org/preferences-service;1"]
                                 .getService(SpecialPowers.Ci.nsIPrefService);
  var branch = prefService.getBranch("media.");
  var oldDefault = 2;
  var oldAuto = 3;
  var oldAppleMedia = undefined;
  var oldGStreamer = undefined;
  var oldOpus = undefined;

  try { oldAppleMedia = SpecialPowers.getBoolPref("media.apple.mp3.enabled"); } catch(ex) { }
  try { oldGStreamer = SpecialPowers.getBoolPref("media.gstreamer.enabled"); } catch(ex) { }
  try { oldDefault   = SpecialPowers.getIntPref("media.preload.default"); } catch(ex) { }
  try { oldAuto      = SpecialPowers.getIntPref("media.preload.auto"); } catch(ex) { }
  try { oldOpus      = SpecialPowers.getBoolPref("media.opus.enabled"); } catch(ex) { }

  SpecialPowers.setIntPref("media.preload.default", 2); 
  SpecialPowers.setIntPref("media.preload.auto", 3); 
  
  if (oldOpus !== undefined)
    SpecialPowers.setBoolPref("media.opus.enabled", true);
  if (oldGStreamer !== undefined)
    SpecialPowers.setBoolPref("media.gstreamer.enabled", true);
  if (oldAppleMedia !== undefined)
    SpecialPowers.setBoolPref("media.apple.mp3.enabled", true);

  window.addEventListener("unload", function() {
    if (oldGStreamer !== undefined)
      SpecialPowers.setBoolPref("media.gstreamer.enabled", oldGStreamer);
    if (oldAppleMedia !== undefined)
      SpecialPowers.setBoolPref("media.apple.mp3.enabled", oldAppleMedia);
    SpecialPowers.setIntPref("media.preload.default", oldDefault);
    SpecialPowers.setIntPref("media.preload.auto", oldAuto);
    if (oldOpus !== undefined)
      SpecialPowers.setBoolPref("media.opus.enabled", oldOpus);
  }, false);
 })();
