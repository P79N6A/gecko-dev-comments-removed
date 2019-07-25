




































EXPORTED_SYMBOLS = ["Sharing"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/async.js");
Cu.import("resource://weave/identity.js");

Function.prototype.async = Async.sugar;

function Api(dav) {
  this._dav = dav;
}

Api.prototype = {
  shareWithUsers: function Api_shareWithUsers(path, users, folder, onComplete) {
    return this._shareGenerator.async(this,
                               onComplete,
                               path,
                               users, folder);
  },

  getShares: function Api_getShares(onComplete) {
    return this._getShareGenerator.async(this, onComplete);
  },
  
  _getShareGenerator: function Api__getShareGenerator() {
    let self = yield;
    let id = ID.get(this._dav.identity);
    
    this._dav.formPost("/api/share/get.php", ("uid=" + escape(id.username) +
                                              "&password=" + escape(id.password)),
                                              self.cb);
    let xhr = yield;
    self.done(xhr.responseText);
  },

  _shareGenerator: function Api__shareGenerator(path, users, folder) {
    let self = yield;
    let id = ID.get(this._dav.identity);

    let cmd = {"version" : 1,
               "directory" : path,
               "share_to_users" : users};
    let jsonSvc = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    let json = jsonSvc.encode(cmd);

    this._dav.formPost("/api/share/",
                   ("cmd=" + escape(json) +
                    "&uid=" + escape(id.username) +
                    "&password=" + escape(id.password) +
                    "&name=" + escape(folder)),
                   self.cb);
    let xhr = yield;

    let retval;

    if (xhr.status == 200) {
      if (xhr.responseText == "OK") {
        retval = {wasSuccessful: true};
      } else {
        retval = {wasSuccessful: false,
                  errorText: xhr.responseText};
      }
    } else {
      retval = {wasSuccessful: false,
                errorText: "Server returned status " + xhr.status};
    }

    self.done(retval);
  }
};

Sharing = {
  Api: Api
};
