


















var PingPongConnection = (function () {
  function PingPongConnection(url, onlySend) {
    this.url = url;
    this.onData = null;
    this.onError = null;
    this.currentXhr = null;
    this.closed = false;

    if (!onlySend) {
      this.idle();
    }
  }

  PingPongConnection.prototype = {
    idle: function () {
      function requestIncoming(connection) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', connection.url + '?idle', true);
        xhr.onload = function () {
          if (xhr.status === 204 &&
              xhr.getResponseHeader('X-PingPong-Error') === 'timeout') {
            requestIncoming(connection);
            return;
          }
          if (xhr.status === 200) {
            var result;
            if (connection.onData) {
              var response = xhr.responseText;
              result = connection.onData(response ? JSON.parse(response) : undefined);
            }
            if (xhr.getResponseHeader('X-PingPong-Async') === '1') {
              requestIncoming(connection);
            } else {
              sendResponse(connection, result);
            }
            return;
          }

          if (connection.onError) {
            connection.onError(xhr.statusText);
          }
        };
        xhr.onerror = function () {
          if (connection.onError) {
            connection.onError(xhr.error);
          }
        };
        xhr.send();
        connection.currentXhr = xhr;
      }
      function sendResponse(connection, result) {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', connection.url + '?response', false);
        xhr.onload = function () {
          if (xhr.status !== 204) {
            if (connection.onError) {
              connection.onError(xhr.statusText);
            }
          }
          requestIncoming(connection);
        };
        xhr.onerror = function () {
          if (connection.onError) {
            connection.onError(xhr.error);
          }
        };
        xhr.send(result === undefined ? '' : JSON.stringify(result));
        connection.currentXhr = xhr;
      }
      requestIncoming(this);
    },
    send: function (data, async, timeout) {
      if (this.closed) {
        throw new Error('connection closed');
      }

      async = !!async;
      timeout |= 0;

      var encoded = data === undefined ? '' : JSON.stringify(data);
      if (async) {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', this.url + '?async', true);
        xhr.send(encoded);
        return;
      } else {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', this.url, false);
        if (timeout > 0) {
          xhr.setRequestHeader('X-PingPong-Timeout', timeout);
        }
        xhr.send(encoded);
        if (xhr.status === 204 &&
          xhr.getResponseHeader('X-PingPong-Error') === 'timeout') {
          throw new Error('sync request timeout');
        }
        var response = xhr.responseText;
        return response ? JSON.parse(response) : undefined;
      }
    },
    close: function () {
      if (this.currentXhr) {
        this.currentXhr.abort();
        this.currentXhr = null;
      }
      this.closed = true;
    }
  };

  return PingPongConnection;
})();
