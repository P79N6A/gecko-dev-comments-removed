function check_webm(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "no", type);
  }

  
  check("video/webm", "maybe");
  check("audio/webm", "maybe");

  
  check("audio/webm; codecs=vorbis", "probably");
  check("video/webm; codecs=vorbis", "probably");
  check("video/webm; codecs=vorbis,vp8", "probably");
  check("video/webm; codecs=vorbis,vp8.0", "probably");
  check("video/webm; codecs=\"vorbis,vp8\"", "probably");
  check("video/webm; codecs=\"vorbis,vp8.0\"", "probably");
  check("video/webm; codecs=\"vp8, vorbis\"", "probably");
  check("video/webm; codecs=\"vp8.0, vorbis\"", "probably");
  check("video/webm; codecs=vp8", "probably");
  check("video/webm; codecs=vp8.0", "probably");
  check("video/webm; codecs=\"vp9, vorbis\"", "probably");
  check("video/webm; codecs=\"vp9.0, vorbis\"", "probably");
  check("video/webm; codecs=\"vp9, opus\"", "probably");
  check("video/webm; codecs=\"vp9.0, opus\"", "probably");
  check("video/webm; codecs=vp9", "probably");
  check("video/webm; codecs=vp9.0", "probably");

  
  check("video/webm; codecs=xyz", "");
  check("video/webm; codecs=xyz,vorbis", "");
  check("video/webm; codecs=vorbis,xyz", "");
}
