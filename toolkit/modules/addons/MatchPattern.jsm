



"use strict";

const Cu = Components.utils;

const EXPORTED_SYMBOLS = ["MatchPattern"];

const PERMITTED_SCHEMES = ["http", "https", "file", "ftp"];



function globToRegexp(pat, allowQuestion)
{
  
  pat = pat.replace(/[.+^${}()|[\]\\]/g, "\\$&");

  if (allowQuestion) {
    pat = pat.replace(/\?/g, ".");
  } else {
    pat = pat.replace(/\?/g, "\\?");
  }
  pat = pat.replace(/\*/g, ".*");
  return new RegExp("^" + pat + "$");
}



function SingleMatchPattern(pat)
{
  if (pat == "<all_urls>") {
    this.scheme = PERMITTED_SCHEMES;
    this.host = "*";
    this.path = new RegExp('.*');
  } else if (!pat) {
    this.scheme = [];
  } else {
    let re = new RegExp("^(http|https|file|ftp|\\*)://(\\*|\\*\\.[^*/]+|[^*/]+|)(/.*)$");
    let match = re.exec(pat);
    if (!match) {
      Cu.reportError(`Invalid match pattern: '${pat}'`);
      this.scheme = [];
      return;
    }

    if (match[1] == '*') {
      this.scheme = ["http", "https"];
    } else {
      this.scheme = [match[1]];
    }
    this.host = match[2];
    this.path = globToRegexp(match[3], false);

    
    if (this.host == "" && this.scheme[0] != "file") {
      Cu.reportError(`Invalid match pattern: '${pat}'`);
      this.scheme = [];
      return;
    }
  }
}

SingleMatchPattern.prototype = {
  matches(uri) {
    if (this.scheme.indexOf(uri.scheme) == -1) {
      return false;
    }

    
    if (this.host == '*') {
      
    } else if (this.host[0] == '*') {
      
      let suffix = this.host.substr(2);
      if (uri.host != suffix && !uri.host.endsWith("." + suffix)) {
        return false;
      }
    } else {
      if (this.host != uri.host) {
        return false;
      }
    }

    if (!this.path.test(uri.path)) {
      return false;
    }

    return true;
  }
};

function MatchPattern(pat)
{
  this.pat = pat;
  if (!pat) {
    this.matchers = [];
  } else if (pat instanceof String || typeof(pat) == "string") {
    this.matchers = [new SingleMatchPattern(pat)];
  } else {
    this.matchers = [for (p of pat) new SingleMatchPattern(p)];
  }
}

MatchPattern.prototype = {
  
  matches(uri) {
    for (let matcher of this.matchers) {
      if (matcher.matches(uri)) {
        return true;
      }
    }
    return false;
  },

  serialize() {
    return this.pat;
  },
};
