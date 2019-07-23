






































const kXHTMLNamespaceURI  = "http://www.w3.org/1999/xhtml";


const kTileExponentWidth  = 7;
const kTileExponentHeight = 7;
const kTileWidth  = Math.pow(2, kTileExponentWidth);   
const kTileHeight = Math.pow(2, kTileExponentHeight);  
const kLazyRoundTimeCap = 500;    


function bind(f, thisObj) {
  return function() {
    return f.apply(thisObj, arguments);
  };
}

function bindSome(instance, methodNames) {
  for each (let methodName in methodNames)
    if (methodName in instance)
      instance[methodName] = bind(instance[methodName], instance);
}

function bindAll(instance) {
  for (let key in instance)
    if (instance[key] instanceof Function)
      instance[key] = bind(instance[key], instance);
}














function TileManager(appendTile, removeTile, browserView) {
  
  this._browserView = browserView;

  
  this._appendTile = appendTile;
  this._removeTile = removeTile;

  
  let self = this;
  this._tileCache = new TileManager.TileCache(function(tile) { self._removeTileSafe(tile); },
                                              -1, -1, 110);

  
  
  this._criticalRect = null;

  
  
  this._browser = null;

  
  
  
  this._pageLoadResizerTimeout = 0;

  
  this._idleTileCrawlerTimeout = 0;

  
  this._crawler = null;

  
  
  
  this._pageLoadMaxRight = 0;
  this._pageLoadMaxBottom = 0;

  
  this._needToPanToTop = false;
}

TileManager.prototype = {

  setBrowser: function setBrowser(b) { this._browser = b; },

  
  
  viewportChangeHandler: function viewportChangeHandler(viewportRect,
                                                        criticalRect,
                                                        boundsSizeChanged,
                                                        dirtyAll) {
    
    dump("***vphandler***\n");
    dump(viewportRect.toString() + "\n");
    dump(criticalRect.toString() + "\n");
    dump(boundsSizeChanged + "\n");
    dump(dirtyAll + "\n***************\n");
    

    let tc = this._tileCache;

    tc.iBound = Math.ceil(viewportRect.right / kTileWidth);
    tc.jBound = Math.ceil(viewportRect.bottom / kTileHeight);

    if (!criticalRect || !criticalRect.equals(this._criticalRect)) {
      this.beginCriticalMove(criticalRect);
      this.endCriticalMove(criticalRect, !boundsSizeChanged);
    }

    if (boundsSizeChanged) {
      
      this.dirtyRects([viewportRect.clone()], true);
    }
  },

  dirtyRects: function dirtyRects(rects, doCriticalRender) {
    let criticalIsDirty = false;
    let criticalRect = this._criticalRect;

    for each (let rect in rects) {
      this._tileCache.forEachIntersectingRect(rect, false, this._dirtyTile, this);

      if (criticalRect && rect.intersects(criticalRect))
        criticalIsDirty = true;
    }

    if (criticalIsDirty && doCriticalRender)
      this.criticalRectPaint();
  },

  criticalRectPaint: function criticalRectPaint() {
    let cr = this._criticalRect;

    if (cr) {
      let [ctrx, ctry] = cr.centerRounded();
      this.recenterEvictionQueue(ctrx, ctry);
      this._renderAppendHoldRect(cr);
    }
  },

  beginCriticalMove2: function beginCriticalMove(destCriticalRect) {
    let start = Date.now();
    function appendNonDirtyTile(tile) {
      if (!tile.isDirty())
        this._appendTileSafe(tile);
    }

    if (destCriticalRect)
      this._tileCache.forEachIntersectingRect(destCriticalRect, false, appendNonDirtyTile, this);
    let end = Date.now();
    dump("start: " + (end-start) + "\n")
  },

  beginCriticalMove: function beginCriticalMove(destCriticalRect) {
  






    let start = Date.now();

    if (destCriticalRect) {

      let rect = destCriticalRect;

      let create = false;

      
      let visited = {};
      let evictGuard = null;
      if (create) {
	evictGuard = function evictGuard(tile) {
	  return !visited[tile.toString()];
	};
      }

      let starti = rect.left  >> kTileExponentWidth;
      let endi   = rect.right >> kTileExponentWidth;

      let startj = rect.top    >> kTileExponentHeight;
      let endj   = rect.bottom >> kTileExponentHeight;

      let tile = null;
      let tc = this._tileCache;

      for (var j = startj; j <= endj; ++j) {
	for (var i = starti; i <= endi; ++i) {

	  

	  
	  
	  if (0 <= i && 0 <= j && i <= tc.iBound && j <= tc.jBound) {
	    
	    break;
	  }

	  tile = null;

	  
	  if (!!(tc._tiles[i] && tc._tiles[i][j])) {
	    tile = tc._tiles[i][j];
	  } else if (create) {
	    
	    tile = tc._createTile(i, j, evictionGuard);
	    if (tile) tile.markDirty();
	  }

	  if (tile) {
	    visited[tile.toString()] = true;
	    
	    
	    
	    if (!tile._dirtyTileCanvas) {
	      
	      if (!tile._appended) {
		let astart = Date.now();
		this._appendTile(tile);
		tile._appended = true;
		let aend = Date.now();
		dump("append: " + (aend - astart) + "\n");
	      }
	    }
	    
	  }
	}
      }
    }

    let end = Date.now();
    dump("start: " + (end-start) + "\n")
  },

  endCriticalMove: function endCriticalMove(destCriticalRect, doCriticalPaint) {
    let start = Date.now();

    let tc = this._tileCache;
    let cr = this._criticalRect;

    let dcr = destCriticalRect.clone();

    let f = function releaseOldTile(tile) {
      
      if (!tile.boundRect.intersects(dcr))
        tc.releaseTile(tile);
    }

    if (cr)
      tc.forEachIntersectingRect(cr, false, f, this);

    this._holdRect(destCriticalRect);

    if (cr)
      cr.copyFrom(destCriticalRect);
    else
      this._criticalRect = cr = destCriticalRect;

    let crpstart = Date.now();
    if (doCriticalPaint)
      this.criticalRectPaint();
    dump(" crp: " + (Date.now() - crpstart) + "\n");

    let end = Date.now();
    dump("end: " + (end - start) + "\n");
  },

  restartLazyCrawl: function restartLazyCrawl(startRectOrQueue) {
    if (!startRectOrQueue || startRectOrQueue instanceof Array) {
      this._crawler = new TileManager.CrawlIterator(this._tileCache);

      if (startRectOrQueue) {
        let len = startRectOrQueue.length;
        for (let k = 0; k < len; ++k)
          this._crawler.enqueue(startRectOrQueue[k].i, startRectOrQueue[k].j);
      }
    } else {
      this._crawler = new TileManager.CrawlIterator(this._tileCache, startRectOrQueue);
    }

    if (!this._idleTileCrawlerTimeout)
      this._idleTileCrawlerTimeout = setTimeout(this._idleTileCrawler, 2000, this);
  },

  stopLazyCrawl: function stopLazyCrawl() {
    this._idleTileCrawlerTimeout = 0;
    this._crawler = null;

    let cr = this._criticalRect;
    if (cr) {
      let [ctrx, ctry] = cr.centerRounded();
      this.recenterEvictionQueue(ctrx, ctry);
    }
  },

  recenterEvictionQueue: function recenterEvictionQueue(ctrx, ctry) {
    let ctri = ctrx >> kTileExponentWidth;
    let ctrj = ctry >> kTileExponentHeight;

    function evictFarTiles(a, b) {
      let dista = Math.max(Math.abs(a.i - ctri), Math.abs(a.j - ctrj));
      let distb = Math.max(Math.abs(b.i - ctri), Math.abs(b.j - ctrj));
      return dista - distb;
    }

    this._tileCache.sortEvictionQueue(evictFarTiles);
  },

  _renderTile: function _renderTile(tile) {
    if (tile.isDirty())
      tile.render(this._browser, this._browserView);
  },

  _appendTileSafe: function _appendTileSafe(tile) {
    if (!tile._appended) {
      this._appendTile(tile);
      tile._appended = true;
    }
  },

  _removeTileSafe: function _removeTileSafe(tile) {
    if (tile._appended) {
      this._removeTile(tile);
      tile._appended = false;
    }
  },

  _dirtyTile: function _dirtyTile(tile) {
    if (!this._criticalRect || !tile.boundRect.intersects(this._criticalRect))
      this._removeTileSafe(tile);

    tile.markDirty();

    if (this._crawler)
      this._crawler.enqueue(tile.i, tile.j);
  },

  _holdRect: function _holdRect(rect) {
    this._tileCache.holdTilesIntersectingRect(rect);
  },

  _releaseRect: function _releaseRect(rect) {
    this._tileCache.releaseTilesIntersectingRect(rect);
  },

  _renderAppendHoldRect: function _renderAppendHoldRect(rect) {
    function renderAppendHoldTile(tile) {
      if (tile.isDirty())
        this._renderTile(tile);

      this._appendTileSafe(tile);
      this._tileCache.holdTile(tile);
    }

    this._tileCache.forEachIntersectingRect(rect, true, renderAppendHoldTile, this);
  },

  _idleTileCrawler: function _idleTileCrawler(self) {
    if (!self) self = this;
    dump('crawl pass.\n');
    let itered = 0, rendered = 0;

    let start = Date.now();
    let comeAgain = true;

    while ((Date.now() - start) <= kLazyRoundTimeCap) {
      let tile = self._crawler.next();

      if (!tile) {
        comeAgain = false;
        break;
      }

      if (tile.isDirty()) {
        self._renderTile(tile);
        ++rendered;
      }
      ++itered;
    }

    dump('crawl itered:' + itered + ' rendered:' + rendered + '\n');

    if (comeAgain) {
      self._idleTileCrawlerTimeout = setTimeout(self._idleTileCrawler, 2000, self);
    } else {
      self.stopLazyCrawl();
      dump('crawl end\n');
    }
  }

};













TileManager.TileCache = function TileCache(onBeforeTileDetach, iBound, jBound, capacity) {
  if (arguments.length <= 3 || capacity < 0)
    capacity = Infinity;

  
  
  
  
  
  
  
  this._tiles = [];

  
  
  this._tilePool = (capacity == Infinity) ? new Array() : new Array(capacity);

  this._capacity = capacity;
  this._nTiles = 0;
  this._numFree = 0;

  this._onBeforeTileDetach = onBeforeTileDetach;

  this.iBound = iBound;
  this.jBound = jBound;
};

TileManager.TileCache.prototype = {

  get size() { return this._nTiles; },
  get numFree() { return this._numFree; },

  
  
  
  
  evictionCmp: function freeTilesLast(a, b) {
    if (a.free == b.free) return (a.j == b.j) ? b.i - a.i : b.j - a.j;
    return (a.free) ? 1 : -1;
  },

  getCapacity: function getCapacity() { return this._capacity; },

  setCapacity: function setCapacity(newCap, skipEvictionQueueSort) {
    if (newCap < 0)
      throw "Cannot set a negative tile cache capacity";

    if (newCap == Infinity) {
      this._capacity = Infinity;
      return;
    } else if (this._capacity == Infinity) {
      
      this._capacity = this._tilePool.length;
    }

    let rem = null;

    if (newCap < this._capacity) {
      
      
      
      
      
      
      if (!skipEvictionQueueSort)
        this.sortEvictionQueue();

      rem = this._tilePool.splice(newCap);

    } else {
      
      this._tilePool.push.apply(this._tilePool, new Array(newCap - this._capacity));
    }

    
    let nTilesDeleted = this._nTiles - newCap;
    if (nTilesDeleted > 0) {
      let nFreeDeleted = 0;
      for (let k = 0; k < nTilesDeleted; ++k) {
        if (rem[k].free)
          nFreeDeleted++;

        this._detachTile(rem[k].i, rem[k].j);
      }

      this._nTiles -= nTilesDeleted;
      this._numFree -= nFreeDeleted;
    }

    this._capacity = newCap;
  },

  _isOccupied: function _isOccupied(i, j) {
    return !!(this._tiles[i] && this._tiles[i][j]);
  },

  _detachTile: function _detachTile(i, j) {
    let tile = null;
    if (this._isOccupied(i, j)) {
      tile = this._tiles[i][j];

      if (this._onBeforeTileDetach)
        this._onBeforeTileDetach(tile);

      this.releaseTile(tile);
      delete this._tiles[i][j];
    }
    return tile;
  },

  _reassignTile: function _reassignTile(tile, i, j) {
    this._detachTile(tile.i, tile.j);    
    tile.init(i, j);                     
    this._tiles[i][j] = tile;            
    return tile;
  },

  _evictTile: function _evictTile(evictionGuard) {
    let k = this._nTiles - 1;
    let pool = this._tilePool;
    let victim = null;

    for (; k >= 0; --k) {
      if (pool[k].free &&
          (!evictionGuard || evictionGuard(pool[k])))
      {
        victim = pool[k];
        break;
      }
    }

    return victim;
  },

  _createTile: function _createTile(i, j, evictionGuard) {
    if (!this._tiles[i])
      this._tiles[i] = [];

    let tile = null;

    if (this._nTiles < this._capacity) {
      
      tile = new TileManager.Tile(i, j);
      this._tiles[i][j] = tile;
      this._tilePool[this._nTiles++] = tile;
      this._numFree++;

    } else {
      
      dump("\nevicting\n");
      tile = this._evictTile(evictionGuard);
      if (tile)
        this._reassignTile(tile, i, j);
    }

    return tile;
  },

  inBounds: function inBounds(i, j) {
    return 0 <= i && 0 <= j && i <= this.iBound && j <= this.jBound;
  },

  sortEvictionQueue: function sortEvictionQueue(cmp) {
    if (!cmp) cmp = this.evictionCmp;
    this._tilePool.sort(cmp);
  },

  













  getTile: function getTile(i, j, create, evictionGuard) {
    if (!this.inBounds(i, j))
      return null;

    let tile = null;

    if (this._isOccupied(i, j)) {
      tile = this._tiles[i][j];
    } else if (create) {
      tile = this._createTile(i, j, evictionGuard);
      if (tile) tile.markDirty();
    }

    return tile;
  },

  







  tileFromPoint: function tileFromPoint(x, y, create) {
    let i = x >> kTileExponentWidth;
    let j = y >> kTileExponentHeight;

    return this.getTile(i, j, create);
  },

  



  holdTile: function holdTile(tile) {
    if (tile && tile.free) {
      tile._hold();
      this._numFree--;
      return true;
    }
    return false;
  },

  



  releaseTile: function releaseTile(tile) {
    if (tile && !tile.free) {
      tile._release();
      this._numFree++;
      return true;
    }
    return false;
  },

  
  
  



  tilesIntersectingRect: function tilesIntersectingRect(rect, create) {
    let dx = (rect.right % kTileWidth) - (rect.left % kTileWidth);
    let dy = (rect.bottom % kTileHeight) - (rect.top % kTileHeight);
    let tiles = [];

    for (let y = rect.top; y <= rect.bottom - dy; y += kTileHeight) {
      for (let x = rect.left; x <= rect.right - dx; x += kTileWidth) {
        let tile = this.tileFromPoint(x, y, create);
        if (tile)
          tiles.push(tile);
      }
    }

    return tiles;
  },

  forEachIntersectingRect: function forEachIntersectingRect(rect, create, fn, thisObj) {
    let visited = {};
    let evictGuard = null;
    if (create) {
      evictGuard = function evictGuard(tile) {
        return !visited[tile.toString()];
      };
    }

    let starti = rect.left  >> kTileExponentWidth;
    let endi   = rect.right >> kTileExponentWidth;

    let startj = rect.top    >> kTileExponentHeight;
    let endj   = rect.bottom >> kTileExponentHeight;

    let tile = null;
    for (var j = startj; j <= endj; ++j) {
      for (var i = starti; i <= endi; ++i) {
        tile = this.getTile(i, j, create, evictGuard);
        if (tile) {
          visited[tile.toString()] = true;
          fn.call(thisObj, tile);
        }
      }
    }
  },

  holdTilesIntersectingRect: function holdTilesIntersectingRect(rect) {
    this.forEachIntersectingRect(rect, false, this.holdTile, this);
  },

  releaseTilesIntersectingRect: function releaseTilesIntersectingRect(rect) {
    this.forEachIntersectingRect(rect, false, this.releaseTile, this);
  }

};



TileManager.Tile = function Tile(i, j) {
  
  this._canvas = document.createElementNS(kXHTMLNamespaceURI, "canvas");
  this._canvas.setAttribute("width", String(kTileWidth));
  this._canvas.setAttribute("height", String(kTileHeight));
  this._canvas.setAttribute("moz-opaque", "true");
  

  this.init(i, j);  
};

TileManager.Tile.prototype = {

  
  
  
  init: function init(i, j) {
    if (!this.boundRect)
      this.boundRect = new wsRect(i * kTileWidth, j * kTileHeight, kTileWidth, kTileHeight);
    else
      this.boundRect.setRect(i * kTileWidth, j * kTileHeight, kTileWidth, kTileHeight);

    
    this.i = i;
    this.j = j;

    
    this._dirtyTileCanvas = false;

    
    this._dirtyTileCanvasRect = null;

    
    
    this._appended = false;

    
    
    
    this.free = true;
  },

  
  get x() { return this.boundRect.left; },
  get y() { return this.boundRect.top; },

  
  
  getContentImage: function getContentImage() { return this._canvas; },

  isDirty: function isDirty() { return this._dirtyTileCanvas; },

  



  markDirty: function markDirty() { this.updateDirtyRegion(); },

  unmarkDirty: function unmarkDirty() {
    this._dirtyTileCanvasRect = null;
    this._dirtyTileCanvas = false;
  },

  




  updateDirtyRegion: function updateDirtyRegion(dirtyRect) {
    if (!dirtyRect) {

      if (!this._dirtyTileCanvasRect)
        this._dirtyTileCanvasRect = this.boundRect.clone();
      else
        this._dirtyTileCanvasRect.copyFrom(this.boundRect);

    } else {

      if (!this._dirtyTileCanvasRect)
        this._dirtyTileCanvasRect = dirtyRect.intersect(this.boundRect);
      else if (dirtyRect.intersects(this.boundRect))
        this._dirtyTileCanvasRect.expandToContain(dirtyRect.intersect(this.boundRect));

    }

    
    

    if (this._dirtyTileCanvasRect)
      this._dirtyTileCanvas = true;
  },

  









  render: function render(browser, browserView) {
    if (!this.isDirty())
      this.markDirty();

    let rect = this._dirtyTileCanvasRect;

    let x = rect.left - this.boundRect.left;
    let y = rect.top - this.boundRect.top;

    
    
    

    let ctx = this._canvas.getContext("2d");
    ctx.save();

    browserView.browserToViewportCanvasContext(ctx);

    ctx.translate(x, y);

    let cw = browserView._contentWindow;
    
    ctx.asyncDrawXULElement(browserView._browser,
                   rect.left, rect.top,
                   rect.right - rect.left, rect.bottom - rect.top,
                   "grey",
                   (ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_CARET));

    ctx.restore();

    this.unmarkDirty();
  },

  toString: function toString(more) {
    if (more) {
      return 'Tile(' + [this.i,
                        this.j,
                        "dirty=" + this.isDirty(),
                        "boundRect=" + this.boundRect].join(', ')
               + ')';
    }

    return 'Tile(' + this.i + ', ' + this.j + ')';
  },

  _hold: function hold() { this.free = false; },
  _release: function release() { this.free = true; }

};




















TileManager.CrawlIterator = function CrawlIterator(tileCache, startRect) {
  this._tileCache = tileCache;
  this._stepRect = startRect;

  
  this._visited = {};

  
  
  let visited = this._visited;
  this._notVisited = function(tile) { return !visited[tile]; };

  
  
  this._crawlIndices = !startRect ? null : (function indicesGenerator(rect, tc) {
    let outOfBounds = false;
    while (!outOfBounds) {
      
      rect.left   -= kTileWidth;
      rect.right  += kTileWidth;
      rect.top    -= kTileHeight;
      rect.bottom += kTileHeight;

      let dx = (rect.right % kTileWidth) - (rect.left % kTileWidth);
      let dy = (rect.bottom % kTileHeight) - (rect.top % kTileHeight);

      outOfBounds = true;

      
      for each (let y in [rect.top, rect.bottom]) {
        for (let x = rect.left; x <= rect.right - dx; x += kTileWidth) {
          let i = x >> kTileExponentWidth;
          let j = y >> kTileExponentHeight;
          if (tc.inBounds(i, j)) {
            outOfBounds = false;
            yield [i, j];
          }
        }
      }

      
      for each (let x in [rect.left, rect.right]) {
        for (let y = rect.top; y <= rect.bottom - dy; y += kTileHeight) {
          let i = x >> kTileExponentWidth;
          let j = y >> kTileExponentHeight;
          if (tc.inBounds(i, j)) {
            outOfBounds = false;
            yield [i, j];
          }
        }
      }
    }
  })(this._stepRect, this._tileCache),    

  
  this._queueState = !startRect;
  this._queue = [];

  
  
  this._enqueued = {};
};

TileManager.CrawlIterator.prototype = {
  __iterator__: function() {
    while (true) {
      let tile = this.next();
      if (!tile) break;
      yield tile;
    }
  },

  becomeQueue: function becomeQueue() {
    this._queueState = true;
  },

  unbecomeQueue: function unbecomeQueue() {
    this._queueState = false;
  },

  next: function next() {
    if (this._queueState)
      return this.dequeue();

    let tile = null;

    if (this._crawlIndices) {
      try {
        let [i, j] = this._crawlIndices.next();
        tile = this._tileCache.getTile(i, j, true, this._notVisited);
      } catch (e) {
        if (!(e instanceof StopIteration))
          throw e;
      }
    }

    if (tile) {
      this._visited[tile] = true;
    } else {
      this.becomeQueue();
      return this.next();
    }

    return tile;
  },

  dequeue: function dequeue() {
    let tile = null;
    do {
      let idx = this._queue.shift();
      if (!idx)
      return null;

      delete this._enqueued[idx];
      let [i, j]  = this._unstrIndices(idx);
      tile = this._tileCache.getTile(i, j, false);

    } while (!tile);

    return tile;
  },

  enqueue: function enqueue(i, j) {
    let idx = this._strIndices(i, j);
    if (!this._enqueued[idx]) {
      this._queue.push(idx);
      this._enqueued[idx] = true;
    }
  },

  _strIndices: function _strIndices(i, j) {
    return i + "," + j;
  },

  _unstrIndices: function _unstrIndices(str) {
    return str.split(',');
  }

};
