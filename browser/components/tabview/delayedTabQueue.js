




















































function DelayedTabQueue(callback, options) {
  this._callback = callback;
  this._heartbeatInterval = (options && options.interval) || 500;
  this._heartbeatCap = (options && options.cap) || this._heartbeatInterval / 2;

  this._entries = [];
  this._tabPriorities = new WeakMap();
}

DelayedTabQueue.prototype = {
  _callback: null,
  _entries: null,
  _tabPriorities: null,
  _isPaused: false,
  _heartbeat: null,
  _heartbeatCap: 0,
  _heartbeatInterval: 0,
  _lastExecutionTime: 0,

  
  
  
  pause: function DQ_pause() {
    this._isPaused = true;
    this._stopHeartbeat();
  },

  
  
  
  resume: function DQ_resume() {
    this._isPaused = false;
    this._startHeartbeat();
  },

  
  
  
  
  
  
  push: function DQ_push(tab) {
    let prio = this._getTabPriority(tab);

    if (this._tabPriorities.has(tab)) {
      let oldPrio = this._tabPriorities.get(tab);

      
      if (prio != oldPrio) {
        this._tabPriorities.set(tab, prio);
        this._sortEntries();
      }

      return;
    }

    let shouldDefer = this._isPaused || this._entries.length ||
                      Date.now() - this._lastExecutionTime < this._heartbeatInterval;

    if (shouldDefer) {
      this._tabPriorities.set(tab, prio);

      
      this._entries.push(tab);
      this._sortEntries();
      this._startHeartbeat();
    } else {
      
      this._executeCallback(tab);
    }
  },

  
  
  
  _sortEntries: function DQ__sortEntries() {
    let self = this;

    this._entries.sort(function (left, right) {
      let leftPrio = self._tabPriorities.get(left);
      let rightPrio = self._tabPriorities.get(right);

      return leftPrio - rightPrio;
    });
  },

  
  
  
  
  
  
  _getTabPriority: function DQ__getTabPriority(tab) {
    if (this.parent && (this.parent.isStacked() &&
        !this.parent.isTopOfStack(this) &&
        !this.parent.expanded))
      return 0;

    return 1;
  },

  
  
  
  _startHeartbeat: function DQ__startHeartbeat() {
    if (!this._heartbeat) {
      this._heartbeat = setTimeout(this._checkHeartbeat.bind(this),
                                   this._heartbeatInterval);
    }
  },

  
  
  
  _checkHeartbeat: function DQ__checkHeartbeat() {
    this._heartbeat = null;

    
    if (this._isPaused || !this._entries.length)
      return;

    
    if (UI.isIdle()) {
      let startTime = Date.now();
      let timeElapsed = 0;

      do {
        
        let tab = this._entries.shift();
        this._tabPriorities.delete(tab);
        this._executeCallback(tab);

        
        
        timeElapsed = this._lastExecutionTime - startTime;
      } while (this._entries.length && timeElapsed < this._heartbeatCap);
    }

    
    if (this._entries.length)
      this._startHeartbeat();
  },

  _executeCallback: function DQ__executeCallback(tab) {
    this._lastExecutionTime = Date.now();
    this._callback(tab);
  },

  
  
  
  _stopHeartbeat: function DQ__stopHeartbeat() {
    if (this._heartbeat)
      this._heartbeat = clearTimeout(this._heartbeat);
  },

  
  
  
  
  
  
  remove: function DQ_remove(tab) {
    if (!this._tabPriorities.has(tab))
      return;

    this._tabPriorities.delete(tab);

    let index = this._entries.indexOf(tab);
    if (index > -1)
      this._entries.splice(index, 1);
  },

  
  
  
  clear: function DQ_clear() {
    let tab;

    while (tab = this._entries.shift())
      this._tabPriorities.delete(tab);
  }
};

