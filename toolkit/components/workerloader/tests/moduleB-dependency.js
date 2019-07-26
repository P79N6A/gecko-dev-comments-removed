


exports.B = true;
exports.foo = "foo";


if ("loadedB" in self) {
  throw new Error("B has been evaluted twice");
}
self.loadedB = true;