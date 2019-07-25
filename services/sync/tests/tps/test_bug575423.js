








EnableEngines(["history"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2"};






var history1 = [
  { uri: "http://www.google.com/",
    title: "Google",
    visits: [
      { type: 1,
        date: 0
      },
      { type: 2,
        date: -1
      }
    ]
  },
  { uri: "http://www.cnn.com/",
    title: "CNN",
    visits: [
      { type: 1,
        date: -1
      },
      { type: 2,
        date: -36
      }
    ]
  }
];


var history2 = [
  { uri: "http://www.mozilla.com/",
    title: "Mozilla",
    visits: [
      { type: 1,
        date: 0
      },
      { type: 2,
        date: -36
      }
    ]
  },
  { uri: "http://www.google.com/language_tools?hl=en",
    title: "Language Tools",
    visits: [
      { type: 1, 
        date: 0
      },
      { type: 2, 
        date: -40
      }
    ]
  }
];




Phase('phase1', [
  [History.add, history1],
  [Sync],
  [History.add, history2],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [History.verify, history2]
]);

