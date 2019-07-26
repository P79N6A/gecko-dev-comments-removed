function check_webm(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "", type);
  }

  
  check("video/webm", "maybe");
  check("audio/webm", "maybe");

  
  
  
  
  
  var androidVer = SpecialPowers.Cc['@mozilla.org/system-info;1']
                                  .getService(SpecialPowers.Ci.nsIPropertyBag2)
                                  .getProperty('version');
  info("android version:"+androidVer);
  
  if (navigator.userAgent.indexOf("Mobile") != -1 &&
      navigator.userAgent.indexOf("Android") == -1 && androidVer > 15) {
    var video = ['vp8', 'vp8.0'];
    var audio = ['vorbis'];
  } else {
    var video = ['vp8', 'vp8.0', 'vp9', 'vp9.0'];
    var audio = ['vorbis', 'opus'];
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
