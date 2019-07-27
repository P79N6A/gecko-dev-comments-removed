




var fakeRooms = [
  {
    "roomToken": "_nxD4V4FflQ",
    "roomName": "First Room Name",
    "roomUrl": "http://localhost:3000/rooms/_nxD4V4FflQ",
    "roomOwner": "Alexis",
    "maxSize": 2,
    "creationTime": 1405517546,
    "ctime": 1405517546,
    "expiresAt": 1405534180,
    "participants": []
  },
  {
    "roomToken": "QzBbvGmIZWU",
    "roomName": "Second Room Name",
    "roomUrl": "http://localhost:3000/rooms/QzBbvGmIZWU",
    "roomOwner": "Alexis",
    "maxSize": 2,
    "creationTime": 1405517546,
    "ctime": 1405517546,
    "expiresAt": 1405534180,
    "participants": []
  },
  {
    "roomToken": "3jKS_Els9IU",
    "roomName": "UX Discussion",
    "roomUrl": "http://localhost:3000/rooms/3jKS_Els9IU",
    "roomOwner": "Alexis",
    "maxSize": 2,
    "clientMaxSize": 2,
    "creationTime": 1405517546,
    "ctime": 1405517818,
    "expiresAt": 1405534180,
    "participants": [
       { "displayName": "Alexis", "account": "alexis@example.com", "roomConnectionId": "2a1787a6-4a73-43b5-ae3e-906ec1e763cb" },
       { "displayName": "Adam", "roomConnectionId": "781f012b-f1ea-4ce1-9105-7cfc36fb4ec7" }
     ]
  }
];





navigator.mozLoop = {
  ensureRegistered: function() {},
  getAudioBlob: function(){},
  getLoopPref: function(pref) {
    switch(pref) {
      
      case "gettingStarted.seen":
      case "screenshare.enabled":
        return true;
    }
  },
  setLoopPref: function(){},
  releaseCallData: function() {},
  copyString: function() {},
  contacts: {
    getAll: function(callback) {
      callback(null, []);
    },
    on: function() {}
  },
  rooms: {
    getAll: function(version, callback) {
      callback(null, fakeRooms);
    },
    on: function() {}
  },
  fxAEnabled: true
};
