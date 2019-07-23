




var gSmallTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"320x240.ogv", type:"video/ogg", width:320, height:240 },
  { name: "bug499519.ogv", type:"video/ogg", duration:0.24 },
  { name: "bug506094.ogv", type:"video/ogg", duration:0 },
  { name:"bogus.duh", type:"bogus/duh" }
];





var gPlayTests = [
  
  { name:"r11025_u8_c1.wav", type:"audio/x-wav", duration:1.0 },
  
  { name:"r11025_u8_c1_trunc.wav", type:"audio/x-wav", duration:1.8 },
  
  { name:"r11025_s16_c1_trailing.wav", type:"audio/x-wav", duration:1.0 },
  
  { name:"r16000_u8_c1_list.wav", type:"audio/x-wav", duration:4.2 },
  
  { name:"bug461281.ogg", type:"application/ogg" },
  
  { name:"bug482461.ogv", type:"video/ogg", duration:4.24 },
  
  { name:"bug500311.ogv", type:"video/ogg", duration:1.96 },
  
  { name:"small-shot.ogg", type:"video/ogg" },

  { name:"bogus.duh", type:"bogus/duh" }
];





var gErrorTests = [
  { name: "bug495129.ogv", type:"video/ogg", duration:2.52 },
  { name: "bug498855-1.ogv", type:"video/ogg", duration:0.2 },
  { name: "bug498855-2.ogv", type:"video/ogg", duration:0.2 },
  { name: "bug498855-3.ogv", type:"video/ogg", duration:0.2 },
  { name: "bug501279.ogg", type:"audio/ogg", duration:0 },
  { name: "bug504644.ogv", type:"video/ogg", duration:1.56 },
  { name:"bogus.wav", type:"audio/x-wav" },
  { name:"bogus.ogv", type:"video/ogg" },
  { name:"448636.ogv", type:"video/ogg" },
  { name:"bogus.duh", type:"bogus/duh" }
];


var gSeekTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"seek.ogv", type:"video/ogg", duration:3.966 },
  { name:"bogus.duh", type:"bogus/duh", duration:123 }
];


var gAudioTests = [
  { name:"r11025_s16_c1.wav", type:"audio/x-wav", duration:1.0 },
  { name:"sound.ogg", type:"audio/ogg" },
  { name:"bogus.duh", type:"bogus/duh", duration:123 }
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
       msg + " duration should be around " + test.duration);
  }
}
