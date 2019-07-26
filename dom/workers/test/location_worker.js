



for (var string in self.location) {
  var value = typeof self.location[string] === "function"
              ? self.location[string]()
              : self.location[string];
  postMessage({ "string": string, "value": value });
}
postMessage({ "string": "testfinished" });
