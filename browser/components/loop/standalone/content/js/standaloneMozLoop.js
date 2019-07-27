









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
    











    _postToRoom: function(roomToken, sessionToken, roomData, expectedProps, callback) {
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
        }
      });

      req.done(function(responseData) {
        try {
          callback(null, validate(responseData, expectedProps));
        } catch (err) {
          console.error("Error requesting call info", err.message);
          callback(err);
        }
      }.bind(this));

      req.fail(failureHandler.bind(this, callback));
    },

    







    join: function(roomToken, callback) {
      this._postToRoom(roomToken, null, {
        action: "join",
        displayName: mozL10n.get("rooms_display_name_guest"),
        clientMaxSize: ROOM_MAX_CLIENTS
      }, {
        apiKey: String,
        sessionId: String,
        sessionToken: String,
        expires: Number
      }, callback);
    },

    









    refreshMembership: function(roomToken, sessionToken, callback) {
      this._postToRoom(roomToken, sessionToken, {
        action: "refresh",
        sessionToken: sessionToken
      }, {
        expires: Number
      }, callback);
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
      }, null, callback);
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
    







    setLoopCharPref: function(prefName, value) {
      if (prefName === "seenToS") {
        return;
      }

      localStorage.setItem(prefName, value);
    },

    





    getLoopCharPref: function(prefName) {
      return localStorage.getItem(prefName);
    }
  };

  return StandaloneMozLoop;
})(navigator.mozL10n);
