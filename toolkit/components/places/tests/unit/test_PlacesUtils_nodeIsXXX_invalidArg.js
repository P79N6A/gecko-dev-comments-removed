



function run_test() {
 let nodeIsMethods = [
  "nodeIsFolder",
  "nodeIsBookmark",
  "nodeIsSeparator",
  "nodeIsURI",
  "nodeIsQuery",
  "nodeIsReadOnly",
  "nodeIsHost",
  "nodeIsDay",
  "nodeIsTagQuery",
  "nodeIsContainer",
  "nodeIsHistoryContainer",
  "nodeIsQuery"
 ];
 for (let methodName of nodeIsMethods) {
  Assert.throws(() => PlacesUtils[methodName](true), /Invalid Places node/);
 }
}

