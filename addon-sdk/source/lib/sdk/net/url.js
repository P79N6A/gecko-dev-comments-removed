



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cu, components } = require("chrome");
const { defer } = require("../core/promise");
const { merge } = require("../util/object");

const { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});





function readSync(uri, charset) {
  let { promise, resolve, reject } = defer();

  try {
    resolve(readURISync(uri, charset));
  }
  catch (e) {
    reject("Failed to read: '" + uri + "' (Error Code: " + e.result + ")");
  }

  return promise;
}





function readAsync(uri, charset) {
  let channel = NetUtil.newChannel(uri, charset, null);

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


















function readURI(uri, options) {
  options = merge({
    charset: "UTF-8",
    sync: false
  }, options);

  return options.sync
    ? readSync(uri, options.charset)
    : readAsync(uri, options.charset);
}

exports.readURI = readURI;














function readURISync(uri, charset) {
  charset = typeof charset === "string" ? charset : "UTF-8";

  let channel = NetUtil.newChannel(uri, charset, null);
  let stream = channel.open();

  let count = stream.available();
  let data = NetUtil.readInputStreamToString(stream, count, { charset : charset });

  stream.close();

  return data;
}

exports.readURISync = readURISync;
