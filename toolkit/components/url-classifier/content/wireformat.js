# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:



































this.PROT_VersionParser =
function PROT_VersionParser(type, opt_major, opt_minor, opt_requireMac) {
  this.debugZone = "versionparser";
  this.type = type;
  this.major = 0;
  this.minor = 0;

  this.badHeader = false;

  
  this.mac = false;
  this.macval = "";
  this.macFailed = false;
  this.requireMac = !!opt_requireMac;

  this.update = false;
  this.needsUpdate = false;  
  
  
  
  this.didRead = false;
  if (opt_major)
    this.major = parseInt(opt_major);
  if (opt_minor)
    this.minor = parseInt(opt_minor);
}




PROT_VersionParser.prototype.ImportVersion = function(version) {
  this.major = version.major;
  this.minor = version.minor;

  this.mac = version.mac;
  this.macFailed = version.macFailed;
  this.macval = version.macval;
  
  
}






PROT_VersionParser.prototype.toString = function() {
  var s = "[" + this.type + " " + this.major + "." + this.minor + "]";
  return s;
}






PROT_VersionParser.prototype.versionString = function() {
  return this.major + "." + this.minor;
}







PROT_VersionParser.prototype.toUrl = function() {
  return this.major + ":" + this.minor;
}






PROT_VersionParser.prototype.processOldFormat_ = function(line) {
  if (line[0] != '[' || line.slice(-1) != ']')
    return false;

  var description = line.slice(1, -1);

  
  var tokens = description.split(" ");
  this.type = tokens[0];
  var majorminor = tokens[1].split(".");
  this.major = parseInt(majorminor[0]);
  this.minor = parseInt(majorminor[1]);
  if (isNaN(this.major) || isNaN(this.minor))
    return false;

  if (tokens.length >= 3) {
     this.update = tokens[2] == "update";
  }

  return true;
}






PROT_VersionParser.prototype.fromString = function(line) {
  G_Debug(this, "Calling fromString with line: " + line);
  if (line[0] != '[' || line.slice(-1) != ']')
    return false;

  
  var secondBracket = line.indexOf('[', 1);
  var firstPart = null;
  var secondPart = null;

  if (secondBracket != -1) {
    firstPart = line.substring(0, secondBracket);
    secondPart = line.substring(secondBracket);
    G_Debug(this, "First part: " + firstPart + " Second part: " + secondPart);
  } else {
    firstPart = line;
    G_Debug(this, "Old format: " + firstPart);
  }

  if (!this.processOldFormat_(firstPart))
    return false;

  if (secondPart && !this.processOptTokens_(secondPart))
    return false;

  return true;
}







PROT_VersionParser.prototype.processOptTokens_ = function(line) {
  if (line[0] != '[' || line.slice(-1) != ']')
    return false;
  var description = line.slice(1, -1);
  
  var tokens = description.split(" ");

  for (var i = 0; i < tokens.length; i++) {
    G_Debug(this, "Processing optional token: " + tokens[i]);
    var tokenparts = tokens[i].split("=");
    switch(tokenparts[0]){
    case "mac":
      this.mac = true;
      if (tokenparts.length < 2) {
        G_Debug(this, "Found mac flag but not mac value!");
        return false;
      }
      
      
      this.macval = tokens[i].substr(tokens[i].indexOf("=")+1);
      break;
    default:
      G_Debug(this, "Found unrecognized token: " + tokenparts[0]);
      break;
    }
  }

  return true;
}

#ifdef DEBUG
this.TEST_PROT_WireFormat = function TEST_PROT_WireFormat() {
  if (G_GDEBUG) {
    var z = "versionparser UNITTEST";
    G_Debug(z, "Starting");

    var vp = new PROT_VersionParser("dummy");
    G_Assert(z, vp.fromString("[foo-bar-url 1.234]"),
             "failed to parse old format");
    G_Assert(z, "foo-bar-url" == vp.type, "failed to parse type");
    G_Assert(z, "1" == vp.major, "failed to parse major");
    G_Assert(z, "234" == vp.minor, "failed to parse minor");

    vp = new PROT_VersionParser("dummy");
    G_Assert(z, vp.fromString("[foo-bar-url 1.234][mac=567]"),
             "failed to parse new format");
    G_Assert(z, "foo-bar-url" == vp.type, "failed to parse type");
    G_Assert(z, "1" == vp.major, "failed to parse major");
    G_Assert(z, "234" == vp.minor, "failed to parse minor");
    G_Assert(z, true == vp.mac, "failed to parse mac");
    G_Assert(z, "567" == vp.macval, "failed to parse macval");

    G_Debug(z, "PASSED");
  }
}
#endif
