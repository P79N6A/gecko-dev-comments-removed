function check_dash(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "", type);
  }

  
  check("application/dash+xml", "probably");

  
  check("application/dash+xml; codecs=vorbis", "probably");
  check("application/dash+xml; codecs=vorbis", "probably");
  check("application/dash+xml; codecs=vorbis,vp8", "probably");
  check("application/dash+xml; codecs=vorbis,vp8.0", "probably");
  check("application/dash+xml; codecs=\"vorbis,vp8\"", "probably");
  check("application/dash+xml; codecs=\"vorbis,vp8.0\"", "probably");
  check("application/dash+xml; codecs=\"vp8, vorbis\"", "probably");
  check("application/dash+xml; codecs=\"vp8.0, vorbis\"", "probably");
  check("application/dash+xml; codecs=vp8", "probably");
  check("application/dash+xml; codecs=vp8.0", "probably");

  
  check("application/dash+xml; codecs=xyz", "");
  check("application/dash+xml; codecs=xyz,vorbis", "");
  check("application/dash+xml; codecs=vorbis,xyz", "");
  check("application/dash+xml; codecs=xyz,vp8.0", "");
  check("application/dash+xml; codecs=vp8.0,xyz", "");
}
