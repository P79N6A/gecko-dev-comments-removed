



for (var string in self.location) {
  postMessage({ "string": string, "value": self.location[string] });
}
dump(self.location + " \n");
postMessage({ "string": "testfinished", "value": self.location.toString() });
