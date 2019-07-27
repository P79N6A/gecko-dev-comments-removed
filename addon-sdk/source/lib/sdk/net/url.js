



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Ci, Cu, components } = require("chrome");

const { defer } = require("../core/promise");
const { merge } = require("../util/object");

const { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
















function readURI(uri, options) {
  options = options || {};
  let charset = options.charset || 'UTF-8';

  let channel = NetUtil.newChannel2(uri,
                                    charset,
                                    null,
                                    null,      
                                    Services.scriptSecurityManager.getSystemPrincipal(),
                                    null,      
                                    Ci.nsILoadInfo.SEC_NORMAL,
                                    Ci.nsIContentPolicy.TYPE_OTHER);

  let { promise, resolve, reject } = defer();

  try {
    NetUtil.asyncFetch(channel, function (stream, result) {
      if (components.isSuccessCode(result)) {
        let count = stream.available();
        let data = NetUtil.readInputStreamToString(stream, count, { charset : charset });

        resolve(data);
      } else {
        reject("Failed to read: '" + uri + "' (Error Code: " + result + ")");
      }
    });
  }
  catch (e) {
    reject("Failed to read: '" + uri + "' (Error: " + e.message + ")");
  }

  return promise;
}

exports.readURI = readURI;














function readURISync(uri, charset) {
  charset = typeof charset === "string" ? charset : "UTF-8";

  let channel = NetUtil.newChannel2(uri,
                                    charset,
                                    null,
                                    null,      
                                    Services.scriptSecurityManager.getSystemPrincipal(),
                                    null,      
                                    Ci.nsILoadInfo.SEC_NORMAL,
                                    Ci.nsIContentPolicy.TYPE_OTHER);
  let stream = channel.open();

  let count = stream.available();
  let data = NetUtil.readInputStreamToString(stream, count, { charset : charset });

  stream.close();

  return data;
}

exports.readURISync = readURISync;
