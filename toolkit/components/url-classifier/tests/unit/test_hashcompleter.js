































let basicCompletionSet = [
  {
    hash: "abcdefgh",
    expectCompletion: true,
    table: "test",
    chunkId: 1234,
  },
  {
    hash: "1234",
    expectCompletion: false,
  },
  {
    hash: "\u0000\u0000\u000012312",
    expectCompletion: true,
    table: "test",
    chunkId: 1234,
  }
];


let falseCompletionSet = [
  {
    hash: "1234",
    expectCompletion: false,
  },
  {
    hash: "",
    expectCompletion: false,
  },
  {
    hash: "abc",
    expectCompletion: false,
  }
];



let dupedCompletionSet = [
  {
    hash: "1234",
    expectCompletion: true,
    table: "test",
    chunkId: 1,
  },
  {
    hash: "5678",
    expectCompletion: false,
    table: "test2",
    chunkId: 2,
  },
  {
    hash: "1234",
    expectCompletion: true,
    table: "test",
    chunkId: 1,
  },
  {
    hash: "5678",
    expectCompletion: false,
    table: "test2",
    chunkId: 2
  }
];



let multipleResponsesCompletionSet = [
  {
    hash: "1234",
    expectCompletion: true,
    multipleCompletions: true,
    completions: [
      {
        hash: "123456",
        table: "test1",
        chunkId: 3,
      },
      {
        hash: "123478",
        table: "test2",
        chunkId: 4,
      }
    ],
  }
];





const SIZE_OF_RANDOM_SET = 16;
function getRandomCompletionSet(forceServerError) {
  let completionSet = [];
  let hashPrefixes = [];

  let seed = Math.floor(Math.random() * Math.pow(2, 32));
  dump("Using seed of " + seed + " for random completion set.\n");
  let rand = new LFSRgenerator(seed);

  for (let i = 0; i < SIZE_OF_RANDOM_SET; i++) {
    let completion = { expectCompletion: false, forceServerError: false, _finished: false };

    
    
    let hash;
    let prefix;
    do {
      hash = "";
      let length = 1 + rand.nextNum(5);
      for (let i = 0; i < length; i++)
        hash += String.fromCharCode(rand.nextNum(8));
      prefix = hash.substring(0,4);
    } while (hashPrefixes.indexOf(prefix) != -1);

    hashPrefixes.push(prefix);
    completion.hash = hash;

    if (!forceServerError) {
      completion.expectCompletion = rand.nextNum(1) == 1;
    } else {
      completion.forceServerError = true;
    }
    if (completion.expectCompletion) {
      
      
      completion.table = (rand.nextNum(31)).toString(36);

      completion.chunkId = rand.nextNum(16);
    }
    completionSet.push(completion);
  }

  return completionSet;
}

let completionSets = [basicCompletionSet, falseCompletionSet,
                      dupedCompletionSet, multipleResponsesCompletionSet];
let currentCompletionSet = -1;
let finishedCompletions = 0;

const SERVER_PORT = 8080;
const SERVER_PATH = "/hash-completer";
let server;




const COMPLETE_LENGTH = 32;

let completer = Cc["@mozilla.org/url-classifier/hashcompleter;1"].
                  getService(Ci.nsIUrlClassifierHashCompleter);

let gethashUrl;


let expectedMaxServerCompletionSet = 0;
let maxServerCompletionSet = 0;

function run_test() {
  
  completionSets.push(getRandomCompletionSet(false));
  
  
  expectedMaxServerCompletionSet = completionSets.length;
  
  for (let j = 0; j < 10; ++j) {
    completionSets.push(getRandomCompletionSet(true));
  }

  
  for each (let completionSet in completionSets) {
    for each (let completion in completionSet) {
      
      if (completion.multipleCompletions) {
        for each (let responseCompletion in completion.completions) {
          let numChars = COMPLETE_LENGTH - responseCompletion.hash.length;
          responseCompletion.hash += (new Array(numChars + 1)).join("\u0000");
        }
      }
      else {
        let numChars = COMPLETE_LENGTH - completion.hash.length;
        completion.hash += (new Array(numChars + 1)).join("\u0000");
      }
    }
  }
  do_test_pending();

  server = new HttpServer();
  server.registerPathHandler(SERVER_PATH, hashCompleterServer);

  server.start(-1);
  const SERVER_PORT = server.identity.primaryPort;

  gethashUrl = "http://localhost:" + SERVER_PORT + SERVER_PATH;

  runNextCompletion();
}

function runNextCompletion() {
  
  
  currentCompletionSet++;
  if (currentCompletionSet >= completionSets.length) {
    finish();
    return;
  }

  dump("Now on completion set index " + currentCompletionSet + ", length " +
       completionSets[currentCompletionSet].length + "\n");
  
  finishedCompletions = 0;
  for each (let completion in completionSets[currentCompletionSet]) {
    completer.complete(completion.hash.substring(0,4), gethashUrl,
                       (new callback(completion)));
  }
}

function hashCompleterServer(aRequest, aResponse) {
  let stream = aRequest.bodyInputStream;
  let wrapperStream = Cc["@mozilla.org/binaryinputstream;1"].
                        createInstance(Ci.nsIBinaryInputStream);
  wrapperStream.setInputStream(stream);

  let len = stream.available();
  let data = wrapperStream.readBytes(len);

  
  
  let completedHashes = [];
  let responseText = "";

  function responseForCompletion(x) {
    return x.table + ":" + x.chunkId + ":" + x.hash.length + "\n" + x.hash;
  }
  
  
  let httpStatus = 204;
  for each (let completion in completionSets[currentCompletionSet]) {
    if (completion.expectCompletion &&
        (completedHashes.indexOf(completion.hash) == -1)) {
      completedHashes.push(completion.hash);

      if (completion.multipleCompletions)
        responseText += completion.completions.map(responseForCompletion).join("");
      else
        responseText += responseForCompletion(completion);
    }
    if (completion.forceServerError) {
      httpStatus = 503;
    }
  }

  dump("Server sending response for " + currentCompletionSet + "\n");
  maxServerCompletionSet = currentCompletionSet;
  if (responseText && httpStatus != 503) {
    aResponse.write(responseText);
  } else {
    aResponse.setStatusLine(null, httpStatus, null);
  }
}


function callback(completion) {
  this._completion = completion;
}

callback.prototype = {
  completion: function completion(hash, table, chunkId, trusted) {
    do_check_true(this._completion.expectCompletion);
    if (this._completion.multipleCompletions) {
      for each (let completion in this._completion.completions) {
        if (completion.hash == hash) {
          do_check_eq(JSON.stringify(hash), JSON.stringify(completion.hash));
          do_check_eq(table, completion.table);
          do_check_eq(chunkId, completion.chunkId);

          completion._completed = true;

          if (this._completion.completions.every(function(x) x._completed))
            this._completed = true;

          break;
        }
      }
    }
    else {
      
      do_check_eq(JSON.stringify(hash), JSON.stringify(this._completion.hash));
      do_check_eq(table, this._completion.table);
      do_check_eq(chunkId, this._completion.chunkId);

      this._completed = true;
    }
  },

  completionFinished: function completionFinished(status) {
    finishedCompletions++;
    do_check_eq(!!this._completion.expectCompletion, !!this._completed);
    this._completion._finished = true;

    
    if (currentCompletionSet < completionSets.length &&
        finishedCompletions == completionSets[currentCompletionSet].length) {
      runNextCompletion();
    }
  },
};

function finish() {
  do_check_eq(expectedMaxServerCompletionSet, maxServerCompletionSet);
  server.stop(function() {
    do_test_finished();
  });
}
