function check_webm(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "", type);
  }

  
  check("video/webm", "maybe");
  check("audio/webm", "maybe");

  var video = ['vp8', 'vp8.0', 'vp9', 'vp9.0'];
  var audio = ['vorbis', 'opus'];
  
  
  
  
  
  
  if (navigator.userAgent.indexOf("Mobile") != -1 &&
      navigator.userAgent.indexOf("Android") == -1) {
    
    
    var androidSDKVer = SpecialPowers.Cc['@mozilla.org/system-info;1']
                                     .getService(SpecialPowers.Ci.nsIPropertyBag2)
                                     .getProperty('sdk_version');
    info("android version:"+androidSDKVer);

    
    if (androidSDKVer > 18) {
      video = ['vp8', 'vp8.0', 'vp9', 'vp9.0'];
      audio = ['vorbis'];
    } else if (androidSDKVer > 15) {
      video = ['vp8', 'vp8.0'];
      audio = ['vorbis'];
    }

  }

  audio.forEach(function(acodec) {
    check("audio/webm; codecs=" + acodec, "probably");
    check("video/webm; codecs=" + acodec, "probably");
  });
  video.forEach(function(vcodec) {
    check("video/webm; codecs=" + vcodec, "probably");
    audio.forEach(function(acodec) {
        check("video/webm; codecs=\"" + vcodec + ", " + acodec + "\"", "probably");
        check("video/webm; codecs=\"" + acodec + ", " + vcodec + "\"", "probably");
    });
  });

  
  check("video/webm; codecs=xyz", "");
  check("video/webm; codecs=xyz,vorbis", "");
  check("video/webm; codecs=vorbis,xyz", "");
}
