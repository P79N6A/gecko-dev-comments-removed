






createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");




function mockAddonProvider(aName) {
  let mockProvider = {
    donePromise: null,
    doneResolve: null,
    doneReject: null,
    shutdownPromise: null,
    shutdownResolve: null,

    get name() aName,

    shutdown() {
      this.shutdownResolve();
      return this.donePromise;
    },
  };
  mockProvider.donePromise = new Promise((resolve, reject) => {
    mockProvider.doneResolve = resolve;
    mockProvider.doneReject = reject;
  });
  mockProvider.shutdownPromise = new Promise((resolve, reject) => {
    mockProvider.shutdownResolve = resolve;
  });
  return mockProvider;
};

function run_test() {
  run_next_test();
}


function findInStatus(aStatus, aName) {
  for (let {name, state} of aStatus.state) {
    if (name == aName) {
      return state;
    }
  }
  return null;
}




add_task(function* blockRepoShutdown() {
  
  let realAddonRepo = AMscope.AddonRepository;
  
  let mockRepo = mockAddonProvider("Mock repo");
  AMscope.AddonRepository = mockRepo;

  let mockProvider = mockAddonProvider("Mock provider");

  startupManager();
  AddonManagerPrivate.registerProvider(mockProvider);

  
  let managerDown = promiseShutdownManager();

  
  yield mockProvider.shutdownPromise;
  
  let status = MockAsyncShutdown.status();
  equal(findInStatus(status[0], "Mock provider"), "(none)");
  equal(status[1].name, "AddonRepository: async shutdown");
  equal(status[1].state, "pending");
  
  mockProvider.doneResolve();

  
  yield mockRepo.shutdownPromise;
  
  status = MockAsyncShutdown.status();
  equal(status[0].name, "AddonManager: Waiting for providers to shut down.");
  equal(status[0].state, "Complete");
  equal(status[1].name, "AddonRepository: async shutdown");
  equal(status[1].state, "in progress");

  
  mockRepo.doneResolve();
  yield managerDown;

  
  status = MockAsyncShutdown.status();
  equal(status[0].name, "AddonRepository: async shutdown");
  equal(status[0].state, "done");
});
