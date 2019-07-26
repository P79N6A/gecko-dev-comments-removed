


function log(text) {
  dump("WORKER " + text + "\n");
}

function send(message) {
  self.postMessage(message);
}

function finish() {
  send({kind: "finish"});
}

function ok(condition, description) {
  send({kind: "ok", condition: !!condition, description: "" + description});
}

function is(a, b, description) {
  let outcome = a == b; 
  send({kind: "is", outcome: outcome, description: "" + description, a: "" + a, b: "" + b});
}

function isnot(a, b, description) {
  let outcome = a != b; 
  send({kind: "isnot", outcome: outcome, description: "" + description, a: "" + a, b: "" + b});
}

function info(description) {
  send({kind: "info", description: "" + description});
}
