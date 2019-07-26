


self.onmessage = function onmessage() {
  self.postMessage({result: typeof OS == "undefined"});
};