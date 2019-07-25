







EnableEngines(["passwords"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1",
               "phase4": "profile2" };







var passwords_initial = [
  { hostname: "http://www.example.com",
    submitURL: "http://login.example.com",
    username: "joe",
    password: "SeCrEt123",
    usernameField: "uname",
    passwordField: "pword",
    changes: {
      password: "zippity-do-dah"
    }
  },
  { hostname: "http://www.example.com",
    realm: "login",
    username: "joe",
    password: "secretlogin"
  }
];


var passwords_after_first_update = [
  { hostname: "http://www.example.com",
    submitURL: "http://login.example.com",
    username: "joe",
    password: "zippity-do-dah",
    usernameField: "uname",
    passwordField: "pword"
  },
  { hostname: "http://www.example.com",
    realm: "login",
    username: "joe",
    password: "secretlogin"
  }
];

var passwords_to_delete = [
  { hostname: "http://www.example.com",
    realm: "login",
    username: "joe",
    password: "secretlogin"
  }
];

var passwords_absent = [
  { hostname: "http://www.example.com",
    realm: "login",
    username: "joe",
    password: "secretlogin"
  }
];


var passwords_after_second_update = [
  { hostname: "http://www.example.com",
    submitURL: "http://login.example.com",
    username: "joe",
    password: "zippity-do-dah",
    usernameField: "uname",
    passwordField: "pword"
  }
];





Phase('phase1', [
  [Passwords.add, passwords_initial],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [Passwords.verify, passwords_initial],
  [Passwords.modify, passwords_initial],
  [Passwords.verify, passwords_after_first_update],
  [Sync]
]);

Phase('phase3', [
  [Sync],
  [Passwords.verify, passwords_after_first_update],
  [Passwords.delete, passwords_to_delete],
  [Passwords.verify, passwords_after_second_update],
  [Passwords.verifyNot, passwords_absent],
  [Sync]
]);

Phase('phase4', [
  [Sync],
  [Passwords.verify, passwords_after_second_update],
  [Passwords.verifyNot, passwords_absent]
]);
