










var CharacterCode = [0x0061, 0x0063, 0x0064, 0x0065, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006F, 0x0070, 0x0071, 0x0073, 0x0077, 0x0079, 0x007A];
var NonEscapeCharacter = ["\a", "\c", "\d", "\e", "\g", "\h", "\i", "\j", "\k", "\l", "\m", "\o", "\p", "\q", "\s", "\w", "\y", "\z"];
for (var index = 0; index <= 17; index++) {
  if (String.fromCharCode(CharacterCode[index]) !== NonEscapeCharacter[index]) {
    $ERROR('#' + NonEscapeCharacter[index] + ' ');
  }
}

