









let stateBackup = ss.getBrowserState();

let state = {windows:[{tabs:[{entries:[
  {
    docIdentifier: 1,
    url: "http://example.com",
    children: [
      {
        docIdentifier: 2,
        url: "http://example.com"
      }
    ]
  },
  {
    docIdentifier: 2,
    url: "http://example.com",
    children: [
      {
        docIdentifier: 1,
        url: "http://example.com"
      }
    ]
  }
]}]}]}

function test() {
  registerCleanupFunction(function () {
    ss.setBrowserState(stateBackup);
  });

  
  ss.setBrowserState(JSON.stringify(state));
  ok(true, "Didn't hang!");
}
