








var phases = { "phase1": "profile1",
               "phase2": "profile2" };







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
  },
  { uri: "http://www.mozilla.com/",
    title: "Mozilla",
    visits: [
      { type: 1,
        date: 0
      },
      { type: 1,
        date: -1
      },
      { type: 1,
        date: -20
      },
      { type: 2,
        date: -36
      }
    ]
  }
];


var history_to_delete = [
  { uri: "http://www.cnn.com/" },
  { begin: -24,
    end: -1
  },
  { host: "www.google.com" }
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
  }
];



var history_not = [
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
  },
  { uri: "http://www.mozilla.com/",
    title: "Mozilla",
    visits: [
      { type: 1,
        date: -1
      },
      { type: 1,
        date: -20
      }
    ]
  }
];








Phase('phase1', [
  [History.add, history1],
  [Sync],
]);

Phase('phase2', [
  [Sync],
  [History.verify, history1],
  [History.delete, history_to_delete],
  [History.verify, history2],
  [History.verifyNot, history_not],
  [Sync]
]);

