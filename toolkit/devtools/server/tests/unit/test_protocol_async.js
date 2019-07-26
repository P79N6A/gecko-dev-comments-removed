








let protocol = devtools.require("devtools/server/protocol");
let {method, Arg, Option, RetVal} = protocol;
let events = devtools.require("sdk/event/core");

function simpleHello() {
  return {
    from: "root",
    applicationType: "xpcshell-tests",
    traits: [],
  }
}

let RootActor = protocol.ActorClass({
  typeName: "root",
  initialize: function(conn) {
    protocol.Actor.prototype.initialize.call(this, conn);
    
    this.manage(this);
    this.actorID = "root";
    this.sequence = 0;
  },

  sayHello: simpleHello,

  simpleReturn: method(function() {
    return this.sequence++;
  }, {
    response: { value: RetVal() },
  }),

  promiseReturn: method(function(toWait) {
    
    let deferred = promise.defer();
    let sequence = this.sequence++;

    
    
    let check = () => {
      if ((this.sequence - sequence) < toWait) {
        do_execute_soon(check);
        return;
      }
      deferred.resolve(sequence);
    }
    do_execute_soon(check);

    return deferred.promise;
  }, {
    request: { toWait: Arg(0, "number") },
    response: { value: RetVal("number") },
  }),

  simpleThrow: method(function() {
    throw new Error(this.sequence++);
  }, {
    response: { value: RetVal("number") }
  }),

  promiseThrow: method(function() {
    
    let deferred = promise.defer();
    let sequence = this.sequence++;
    
    do_timeout(150, () => {
      deferred.reject(sequence++);
    });
    return deferred.promise;
  }, {
    response: { value: RetVal("number") },
  })
});

let RootFront = protocol.FrontClass(RootActor, {
  initialize: function(client) {
    this.actorID = "root";
    protocol.Front.prototype.initialize.call(this, client);
    
    this.manage(this);
  }
});

function run_test()
{
  DebuggerServer.createRootActor = RootActor;
  DebuggerServer.init(() => true);

  let trace = connectPipeTracing();
  let client = new DebuggerClient(trace);
  let rootClient;

  client.connect((applicationType, traits) => {
    rootClient = RootFront(client);

    let calls = [];
    let sequence = 0;

    
    
    calls.push(rootClient.promiseReturn(2).then(ret => {
      do_check_eq(sequence, 0); 
      do_check_eq(ret, sequence++); 
    }));

    

    calls.push(rootClient.simpleReturn().then(ret => {
      do_check_eq(sequence, 1); 
      do_check_eq(ret, sequence++); 
    }));

    calls.push(rootClient.simpleReturn().then(ret => {
      do_check_eq(sequence, 2); 
      do_check_eq(ret, sequence++); 
    }));

    calls.push(rootClient.simpleThrow().then(() => {
      do_check_true(false, "simpleThrow shouldn't succeed!");
    }, error => {
      do_check_eq(sequence++, 3); 
    }));

    
    
    
    let deferAfterRejection = promise.defer();

    calls.push(rootClient.promiseThrow().then(() => {
      do_check_true(false, "promiseThrow shouldn't succeed!");
    }, error => {
      do_check_eq(sequence++, 4); 
      do_check_true(true, "simple throw should throw");
      deferAfterRejection.resolve();
    }));

    calls.push(rootClient.simpleReturn().then(ret => {
      return deferAfterRejection.promise.then(function () {
        do_check_eq(sequence, 5); 
        do_check_eq(ret, sequence++); 
      });
    }));

    
    
    calls.push(rootClient.promiseReturn(1).then(ret => {
      return deferAfterRejection.promise.then(function () {
        do_check_eq(sequence, 6); 
        do_check_eq(ret, sequence++); 
      });
    }));

    calls.push(rootClient.simpleReturn().then(ret => {
      return deferAfterRejection.promise.then(function () {
        do_check_eq(sequence, 7); 
        do_check_eq(ret, sequence++); 
      });
    }));

    promise.all(calls).then(() => {
      client.close(() => {
        do_test_finished();
      });
    })
  });
  do_test_pending();
}
