







EnableEngines(["forms"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1",
               "phase4": "profile2" };






var formdata1 = [
   { fieldname: "name",
     value: "xyz",
     date: -1
   },
   { fieldname: "email",
     value: "abc@gmail.com",
     date: -2
   },
   { fieldname: "username",
     value: "joe"
   }
];


var formdata2 = [
   { fieldname: "password",
     value: "secret",
     date: -1
   },
   { fieldname: "city",
     value: "mtview"
   }
];





Phase('phase1', [
  [Formdata.add, formdata1],
  [Formdata.verify, formdata1],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [Formdata.verify, formdata1]
]);

Phase('phase3', [
  [Sync],
  [Windows.add, { private: true }],
  [Formdata.add, formdata2],
  [Formdata.verify, formdata2],
  [Sync],
]);

Phase('phase4', [
  [Sync],
  [Formdata.verify, formdata1],
  [Formdata.verifyNot, formdata2]
]);
