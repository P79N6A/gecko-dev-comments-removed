





























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
function addRandomCompletionSet() {
  let completionSet = [];
  for (let i = 0; i < SIZE_OF_RANDOM_SET; i++) {
    let completion = {};

    
    
    let hash = "";
    let length = 1 + Math.floor(Math.random() * 32);
    for (let i = 0; i < length; i++)
      hash += String.fromCharCode(Math.floor(Math.random() * 256));
    completion.hash = hash;

    completion.expectCompletion = Math.random() < 0.5;
    if (completion.expectCompletion) {
      
      
      completion.table = Math.floor(Math.random() * Math.pow(36, 6)).toString(36);

      completion.chunkId = Math.floor(Math.random() * Math.pow(2, 16));
    }

    completionSet.push(completion);
  }

  completionSets.push(completionSet);
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

function run_test() {
  addRandomCompletionSet();

  
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

  server = new nsHttpServer();
  server.registerPathHandler(SERVER_PATH, hashCompleterServer);

  const SERVER_PORT = 8080;
  server.start(SERVER_PORT);

  completer.gethashUrl = "http://localhost:" + SERVER_PORT + SERVER_PATH;

  runNextCompletion();
}

function doneCompletionSet() {
  do_check_eq(finishedCompletions, completionSets[currentCompletionSet].length);

  for each (let completion in completionSets[currentCompletionSet])
    do_check_true(completion._finished);

  runNextCompletion();
}

function runNextCompletion() {
  currentCompletionSet++;
  finishedCompletions = 0;

  if (currentCompletionSet >= completionSets.length) {
    finish();
    return;
  }

  dump("Now on completion set index " + currentCompletionSet + "\n");
  for each (let completion in completionSets[currentCompletionSet]) {
    completer.complete(completion.hash.substring(0,4),
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
  for each (let completion in completionSets[currentCompletionSet]) {
    if (completion.expectCompletion &&
        (completedHashes.indexOf(completion.hash) == -1)) {
      completedHashes.push(completion.hash);

      if (completion.multipleCompletions)
        responseText += completion.completions.map(responseForCompletion).join("");
      else
        responseText += responseForCompletion(completion);
    }
  }

  
  
  if (responseText)
    aResponse.write(responseText);
  else
    aResponse.setStatusLine(null, 204, null);
}


function callback(completion) {
  this._completion = completion;
}
callback.prototype = {
  completion: function completion(hash, table, chunkId, trusted) {
    
    if (!this._completion.expectCompletion) {
      dump("Did not expect a completion for this result. Provided values:\n" +
           "hash: " + JSON.stringify(hash) + "\ntable: " + table + "chunkId: " +
           chunkId + "\n");
      dump("Actual values:\nhash: " + JSON.stringify(this._completion.hash) +
           "\ntable: " + this._completion.table + "\nchunkId: " +
           this._completion.chunkId + "\n");
    }

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
    do_check_eq(!!this._completion.expectCompletion, !!this._completed);
    this._completion._finished = true;

    finishedCompletions++;
    if (finishedCompletions == completionSets[currentCompletionSet].length)
      doneCompletionSet();
  },
};

function finish() {
  server.stop(function() {
    do_test_finished();
  });
}
