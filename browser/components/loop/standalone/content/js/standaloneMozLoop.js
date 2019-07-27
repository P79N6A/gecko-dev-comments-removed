









var loop = loop || {};
loop.StandaloneMozLoop = (function(mozL10n) {
  "use strict";

  


  var ROOM_MAX_CLIENTS = 2;


  







  function validate(data, schema) {
    if (!schema) {
      return {};
    }

    return new loop.validate.Validator(schema).validate(data);
  }

  







  function failureHandler(callback, jqXHR, textStatus, errorThrown) {
    var jsonErr = jqXHR && jqXHR.responseJSON || {};
    var message = "HTTP " + jqXHR.status + " " + errorThrown;

    
    var err = new Error(message + (jsonErr.error ? "; " + jsonErr.error : ""));
    err.errno = jsonErr.errno;

    callback(err);
  }

  




  var StandaloneMozLoopRooms = function(options) {
    options = options || {};
    if (!options.baseServerUrl) {
      throw new Error("missing required baseServerUrl");
    }

    this._baseServerUrl = options.baseServerUrl;
  };

  StandaloneMozLoopRooms.prototype = {
    








    get: function(roomToken, callback) {
      var req = $.ajax({
        url:         this._baseServerUrl + "/rooms/" + roomToken,
        method:      "GET",
        contentType: "application/json",
        beforeSend: function(xhr) {
          if (this.sessionToken) {
            xhr.setRequestHeader("Authorization", "Basic " + btoa(this.sessionToken));
          }
        }.bind(this)
      });

      req.done(function(responseData) {
        try {
          
          callback(null, validate(responseData, {
            roomOwner: String,
            roomUrl: String
          }));
        } catch (err) {
          console.error("Error requesting call info", err.message);
          callback(err);
        }
      }.bind(this));

      req.fail(failureHandler.bind(this, callback));
    },

    












    _postToRoom: function(roomToken, sessionToken, roomData, expectedProps,
                          async, callback) {
      var req = $.ajax({
        url:         this._baseServerUrl + "/rooms/" + roomToken,
        method:      "POST",
        contentType: "application/json",
        dataType:    "json",
        data: JSON.stringify(roomData),
        beforeSend: function(xhr) {
          if (sessionToken) {
            xhr.setRequestHeader("Authorization", "Basic " + btoa(sessionToken));
          }
        },
        async: async,
        success: function(responseData) {
          console.log("done");
          try {
            callback(null, validate(responseData, expectedProps));
          } catch (err) {
            console.error("Error requesting call info", err.message);
            callback(err);
          }
        }.bind(this),
        error: failureHandler.bind(this, callback)
      });
    },

    







    join: function(roomToken, callback) {
      function callbackWrapper(err, result) {
        
        
        if (result) {
          this.sessionToken = result.sessionToken;
        }

        callback(err, result);
      }

      this._postToRoom(roomToken, null, {
        action: "join",
        displayName: mozL10n.get("rooms_display_name_guest"),
        clientMaxSize: ROOM_MAX_CLIENTS
      }, {
        apiKey: String,
        sessionId: String,
        sessionToken: String,
        expires: Number
      }, true, callbackWrapper.bind(this));
    },

    









    refreshMembership: function(roomToken, sessionToken, callback) {
      this._postToRoom(roomToken, sessionToken, {
        action: "refresh",
        sessionToken: sessionToken
      }, {
        expires: Number
      }, true, callback);
    },

    










    leave: function(roomToken, sessionToken, callback) {
      if (!callback) {
        callback = function(error) {
          if (error) {
            console.error(error);
          }
        };
      }

      
      this._postToRoom(roomToken, sessionToken, {
        action: "leave",
        sessionToken: sessionToken
      }, null, false, callback);
    },

    







    sendConnectionStatus: function(roomToken, sessionToken, status) {
      this._postToRoom(roomToken, sessionToken, {
        action: "status",
        event: status.event,
        state: status.state,
        connections: status.connections,
        sendStreams: status.sendStreams,
        recvStreams: status.recvStreams
      }, null, true, function(error) {
        if (error) {
          console.error(error);
        }
      });
    },

    
    
    on: function() {},
    once: function() {},
    off: function() {}
  };

  var StandaloneMozLoop = function(options) {
    options = options || {};
    if (!options.baseServerUrl) {
      throw new Error("missing required baseServerUrl");
    }

    this._baseServerUrl = options.baseServerUrl;

    this.rooms = new StandaloneMozLoopRooms(options);
  };

  StandaloneMozLoop.prototype = {
    







    setLoopPref: function(prefName, value) {
      if (prefName === "seenToS") {
        return;
      }

      localStorage.setItem(prefName, value);
    },

    





    getLoopPref: function(prefName) {
      return localStorage.getItem(prefName);
    },

    
    
    addConversationContext: function() {},
    setScreenShareState: function() {}
  };

  return StandaloneMozLoop;
})(navigator.mozL10n);
