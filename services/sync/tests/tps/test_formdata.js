








var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1",
               "phase4": "profile2" };





var formdata1 = [
  { fieldname: "testing",
    value: "success",
    date: -1
  },
  { fieldname: "testing",
    value: "failure",
    date: -2
  },
  { fieldname: "username",
    value: "joe"
  }
];

var formdata2 = [
  { fieldname: "testing",
    value: "success",
    date: -1
  },
  { fieldname: "username",
    value: "joe"
  }
];

var formdata_delete = [
  { fieldname: "testing",
    value: "failure"
  }
];





Phase('phase1', [
  [Formdata.add, formdata1],
  [Formdata.verify, formdata1],
  [Sync, SYNC_WIPE_SERVER],
]);

Phase('phase2', [
  [Sync],
  [Formdata.verify, formdata1],
]);






Phase('phase3', [
  [Sync],
  [Formdata.delete, formdata_delete],

  [Formdata.verify, formdata2],
  [Sync],
]);

Phase('phase4', [
  [Sync],
  [Formdata.verify, formdata2],

]);


