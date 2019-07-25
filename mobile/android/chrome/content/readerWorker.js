



importScripts("JSDOMParser.js", "Readability.js");

self.onmessage = function (msg) {
  let uri = msg.data.uri;
  let doc = new JSDOMParser().parse(msg.data.doc);
  new Readability(uri, doc).parse(function (result) {
    postMessage(result);
  });
};
