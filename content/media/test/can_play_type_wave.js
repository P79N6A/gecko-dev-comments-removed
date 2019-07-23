function check_wave(v, enabled) {
  function check(type, expected) {
    is(v.canPlayType(type), enabled ? expected : "no", type);
  }

  
  check("audio/wave", "maybe");
  check("audio/wav", "maybe");
  check("audio/x-wav", "maybe");
  check("audio/x-pn-wav", "maybe");

  
  check("audio/wave; codecs=1", "probably");
  
  check("audio/wave; codecs=", "probably");
  check("audio/wave; codecs=\"\"", "probably");

  
  check("audio/wave; codecs=0", "maybe");
  check("audio/wave; codecs=\"0, 1\"", "maybe");

  
  check("audio/wave; codecs=2", "");
  check("audio/wave; codecs=xyz,0", "");
  check("audio/wave; codecs=0,xyz", "");
  check("audio/wave; codecs=\"xyz, 1\"", "");
  
  check("audio/wave; codecs=,", "");
  check("audio/wave; codecs=\"0, 1,\"", "");
}
