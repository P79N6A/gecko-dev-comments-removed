function check_ogg(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "no", type);
  }

  
  check("video/ogg", "maybe");
  check("audio/ogg", "maybe");
  check("application/ogg", "maybe");

  
  check("audio/ogg; codecs=vorbis", "probably");
  check("video/ogg; codecs=vorbis", "probably");
  check("video/ogg; codecs=vorbis,theora", "probably");
  check("video/ogg; codecs=\"vorbis, theora\"", "probably");
  check("video/ogg; codecs=theora", "probably");

  
  check("video/ogg; codecs=xyz", "");
  check("video/ogg; codecs=xyz,vorbis", "");
  check("video/ogg; codecs=vorbis,xyz", "");
}
