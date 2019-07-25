



importScripts("JSDOMParser.js", "Readability.js");

self.onmessage = function (msg) {
  let uri = msg.data.uri;
  let doc = new JSDOMParser().parse(msg.data.doc);
  let article = new Readability(uri, doc).parse();
  postMessage(article);
};
