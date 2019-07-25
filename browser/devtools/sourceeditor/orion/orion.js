















 
var orion = orion || {};


 
orion.textview = orion.textview || {};





















orion.textview.KeyBinding = (function() {
	var isMac = window.navigator.platform.indexOf("Mac") !== -1;
	
	function KeyBinding (keyCode, mod1, mod2, mod3, mod4) {
		if (typeof(keyCode) === "string") {
			this.keyCode = keyCode.toUpperCase().charCodeAt(0);
		} else {
			this.keyCode = keyCode;
		}
		this.mod1 = mod1 !== undefined && mod1 !== null ? mod1 : false;
		this.mod2 = mod2 !== undefined && mod2 !== null ? mod2 : false;
		this.mod3 = mod3 !== undefined && mod3 !== null ? mod3 : false;
		this.mod4 = mod4 !== undefined && mod4 !== null ? mod4 : false;
	}
	KeyBinding.prototype =  {
		





		match: function (e) {
			if (this.keyCode === e.keyCode) {
				var mod1 = isMac ? e.metaKey : e.ctrlKey;
				if (this.mod1 !== mod1) { return false; }
				if (this.mod2 !== e.shiftKey) { return false; }
				if (this.mod3 !== e.altKey) { return false; }
				if (isMac && this.mod4 !== e.ctrlKey) { return false; }
				return true;
			}
			return false;
		},
		





		equals: function(kb) {
			if (!kb) { return false; }
			if (this.keyCode !== kb.keyCode) { return false; }
			if (this.mod1 !== kb.mod1) { return false; }
			if (this.mod2 !== kb.mod2) { return false; }
			if (this.mod3 !== kb.mod3) { return false; }
			if (this.mod4 !== kb.mod4) { return false; }
			return true;
		} 
	};
	return KeyBinding;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.textview;
	});
}















 
var orion = orion || {};


 
orion.textview = orion.textview || {};






























orion.textview.Ruler = (function() {
	
	function Ruler (rulerLocation, rulerOverview, rulerStyle) {
		this._location = rulerLocation || "left";
		this._overview = rulerOverview || "page";
		this._rulerStyle = rulerStyle;
		this._view = null;
	}
	Ruler.prototype =  {
		




		setView: function (view) {
			if (this._onModelChanged && this._view) {
				this._view.removeEventListener("ModelChanged", this, this._onModelChanged); 
			}
			this._view = view;
			if (this._onModelChanged && this._view) {
				this._view.addEventListener("ModelChanged", this, this._onModelChanged);
			}
		},
		






		getLocation: function() {
			return this._location;
		},
		






		getOverview: function() {
			return this._overview;
		},
		












		getStyle: function(lineIndex) {
		},
		











		getHTML: function(lineIndex) {
		},
		







		getAnnotations: function() {
		},
		






		onClick: function(lineIndex, e) {
		},
		






		onDblClick: function(lineIndex, e) {
		}
	};
	return Ruler;
}());

















orion.textview.LineNumberRuler = (function() {
	
	function LineNumberRuler (rulerLocation, rulerStyle, oddStyle, evenStyle) {
		orion.textview.Ruler.call(this, rulerLocation, "page", rulerStyle);
		this._oddStyle = oddStyle || {style: {backgroundColor: "white"}};
		this._evenStyle = evenStyle || {style: {backgroundColor: "white"}};
		this._numOfDigits = 0;
	}
	LineNumberRuler.prototype = new orion.textview.Ruler(); 
	
	LineNumberRuler.prototype.getStyle = function(lineIndex) {
		if (lineIndex === undefined) {
			return this._rulerStyle;
		} else {
			return lineIndex & 1 ? this._oddStyle : this._evenStyle;
		}
	};
	
	LineNumberRuler.prototype.getHTML = function(lineIndex) {
		if (lineIndex === -1) {
			var model = this._view.getModel();
			return model.getLineCount();
		} else {
			return lineIndex + 1;
		}
	};
	
	LineNumberRuler.prototype._onModelChanged = function(e) {
		var start = e.start;
		var model = this._view.getModel();
		var lineCount = model.getLineCount();
		var numOfDigits = (lineCount+"").length;
		if (this._numOfDigits !== numOfDigits) {
			this._numOfDigits = numOfDigits;
			var startLine = model.getLineAtOffset(start);
			this._view.redrawLines(startLine, lineCount, this);
		}
	};
	return LineNumberRuler;
}());












 
















orion.textview.AnnotationRuler = (function() {
	
	function AnnotationRuler (rulerLocation, rulerStyle, defaultAnnotation) {
		orion.textview.Ruler.call(this, rulerLocation, "page", rulerStyle);
		this._defaultAnnotation = defaultAnnotation;
		this._annotations = [];
	}
	AnnotationRuler.prototype = new orion.textview.Ruler();
	





	AnnotationRuler.prototype.clearAnnotations = function() {
		this._annotations = [];
		var lineCount = this._view.getModel().getLineCount();
		this._view.redrawLines(0, lineCount, this);
		if (this._overviewRuler) {
			this._view.redrawLines(0, lineCount, this._overviewRuler);
		}
	};
	










	AnnotationRuler.prototype.getAnnotation = function(lineIndex) {
		return this._annotations[lineIndex];
	};
	
	AnnotationRuler.prototype.getAnnotations = function() {
		var lines = [];
		for (var prop in this._annotations) {
			var i = prop >>> 0;
			if (this._annotations[i] !== undefined) {
				lines.push(i);
			}
		}
		return lines;
	};
	
	AnnotationRuler.prototype.getStyle = function(lineIndex) {
		switch (lineIndex) {
			case undefined:
				return this._rulerStyle;
			case -1:
				return this._defaultAnnotation ? this._defaultAnnotation.style : null;
			default:
				return this._annotations[lineIndex] && this._annotations[lineIndex].style ? this._annotations[lineIndex].style : null;
		}
	};
		
	AnnotationRuler.prototype.getHTML = function(lineIndex) {
		if (lineIndex === -1) {
			return this._defaultAnnotation ? this._defaultAnnotation.html : "";
		} else {
			return this._annotations[lineIndex] && this._annotations[lineIndex].html ? this._annotations[lineIndex].html : "";
		}
	};
	










	AnnotationRuler.prototype.setAnnotation = function(lineIndex, annotation) {
		if (lineIndex === undefined) { return; }
		this._annotations[lineIndex] = annotation;
		this._view.redrawLines(lineIndex, lineIndex + 1, this);
		if (this._overviewRuler) {
			this._view.redrawLines(lineIndex, lineIndex + 1, this._overviewRuler);
		}
	};
	
	AnnotationRuler.prototype._onModelChanged = function(e) {
		var start = e.start;
		var removedLineCount = e.removedLineCount;
		var addedLineCount = e.addedLineCount;
		var linesChanged = addedLineCount - removedLineCount;
		if (linesChanged) {
			var model = this._view.getModel();
			var startLine = model.getLineAtOffset(start);
			var newLines = [], lines = this._annotations;
			var changed = false;
			for (var prop in lines) {
				var i = prop >>> 0;
				if (!(startLine < i && i < startLine + removedLineCount)) {
					var newIndex = i;
					if (i > startLine) {
						newIndex += linesChanged;
						changed = true;
					}
					newLines[newIndex] = lines[i];
				} else {
					changed = true;
				}
			}
			this._annotations = newLines;
			if (changed) {
				var lineCount = model.getLineCount();
				this._view.redrawLines(startLine, lineCount, this);
				
				if (this._overviewRuler) {
					this._view.redrawLines(0, lineCount, this._overviewRuler);
				}
			}
		}
	};
	return AnnotationRuler;
}());






















orion.textview.OverviewRuler = (function() {
	
	function OverviewRuler (rulerLocation, rulerStyle, annotationRuler) {
		orion.textview.Ruler.call(this, rulerLocation, "document", rulerStyle);
		this._annotationRuler = annotationRuler;
		if (annotationRuler) {
			annotationRuler._overviewRuler = this;
		}
	}
	OverviewRuler.prototype = new orion.textview.Ruler();
	
	OverviewRuler.prototype.getAnnotations = function() {
		return this._annotationRuler.getAnnotations();
	};
		
	OverviewRuler.prototype.getStyle = function(lineIndex) {
		var result, style;
		if (lineIndex === undefined) {
			result = this._rulerStyle || {};
			style = result.style || (result.style = {});
			style.lineHeight = "1px";
			style.fontSize = "1px";
			style.width = "14px";
		} else {
			if (lineIndex !== -1) {
				var annotation = this._annotationRuler.getAnnotation(lineIndex);
				result = annotation.overviewStyle || {};
			} else {
				result = {};
			}
			style = result.style || (result.style = {});
			style.cursor = "pointer";
			style.width = "8px";
			style.height = "3px";
			style.left = "2px";
		}
		return result;
	};
	
	OverviewRuler.prototype.getHTML = function(lineIndex) {
		return "&nbsp;";
	};
		
	OverviewRuler.prototype.onClick = function(lineIndex, e) {
		if (lineIndex === undefined) { return; }
		this._view.setTopIndex(lineIndex);
	};
	return OverviewRuler;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.textview;
	});
}














 
var orion = orion || {};


 
orion.textview = orion.textview || {};
















orion.textview.UndoStack = (function() {
	






	var Change = (function() {
		function Change(offset, text, previousText) {
			this.offset = offset;
			this.text = text;
			this.previousText = previousText;
		}
		Change.prototype = {
			
			undo: function (view, select) {
				this._doUndoRedo(this.offset, this.previousText, this.text, view, select);
			},
			
			redo: function (view, select) {
				this._doUndoRedo(this.offset, this.text, this.previousText, view, select);
			},
			_doUndoRedo: function(offset, text, previousText, view, select) {
				view.setText(text, offset, offset + previousText.length);
				if (select) {
					view.setSelection(offset, offset + text.length);
				}
			}
		};
		return Change;
	}());

	






	var CompoundChange = (function() {
		function CompoundChange (selection, caret) {
			this.selection = selection;
			this.caret = caret;
			this.changes = [];
		}
		CompoundChange.prototype = {
			
			add: function (change) {
				this.changes.push(change);
			},
			
			undo: function (view, select) {
				for (var i=this.changes.length - 1; i >= 0; i--) {
					this.changes[i].undo(view, false);
				}
				if (select) {
					var start = this.selection.start;
					var end = this.selection.end;
					view.setSelection(this.caret ? start : end, this.caret ? end : start);
				}
			},
			
			redo: function (view, select) {
				for (var i = 0; i < this.changes.length; i++) {
					this.changes[i].redo(view, false);
				}
				if (select) {
					var start = this.selection.start;
					var end = this.selection.end;
					view.setSelection(this.caret ? start : end, this.caret ? end : start);
				}
			}
		};
		return CompoundChange;
	}());

	
	function UndoStack (view, size) {
		this.view = view;
		this.size = size !== undefined ? size : 100;
		this.reset();
		view.addEventListener("ModelChanging", this, this._onModelChanging);
		view.addEventListener("Destroy", this, this._onDestroy);
	}
	UndoStack.prototype =  {
		







		add: function (change) {
			if (this.compoundChange) {
				this.compoundChange.add(change);
			} else {
				var length = this.stack.length;
				this.stack.splice(this.index, length-this.index, change);
				this.index++;
				if (this.stack.length > this.size) {
					this.stack.shift();
					this.index--;
					this.cleanIndex--;
				}
			}
		},
		








		markClean: function() {
			this.endCompoundChange();
			this._commitUndo();
			this.cleanIndex = this.index;
		},
		
















		isClean: function() {
			return this.cleanIndex === this.getSize().undo;
		},
		







		canUndo: function() {
			return this.getSize().undo > 0;
		},
		







		canRedo: function() {
			return this.getSize().redo > 0;
		},
		




		endCompoundChange: function() {
			this.compoundChange = undefined;
		},
		








		getSize: function() {
			var index = this.index;
			var length = this.stack.length;
			if (this._undoStart !== undefined) {
				index++;
			}
			return {undo: index, redo: (length - index)};
		},
		







		undo: function() {
			this._commitUndo();
			if (this.index <= 0) {
				return false;
			}
			var change = this.stack[--this.index];
			this._ignoreUndo = true;
			change.undo(this.view, true);
			this._ignoreUndo = false;
			return true;
		},
		







		redo: function() {
			this._commitUndo();
			if (this.index >= this.stack.length) {
				return false;
			}
			var change = this.stack[this.index++];
			this._ignoreUndo = true;
			change.redo(this.view, true);
			this._ignoreUndo = false;
			return true;
		},
		


		reset: function() {
			this.index = this.cleanIndex = 0;
			this.stack = [];
			this._undoStart = undefined;
			this._undoText = "";
			this._ignoreUndo = false;
			this._compoundChange = undefined;
		},
		









		startCompoundChange: function() {
			this._commitUndo();
			var change = new CompoundChange(this.view.getSelection(), this.view.getCaretOffset());
			this.add(change);
			this.compoundChange = change;
		},
		_commitUndo: function () {
			if (this._undoStart !== undefined) {
				if (this._undoStart < 0) {
					this.add(new Change(-this._undoStart, "", this._undoText, ""));
				} else {
					this.add(new Change(this._undoStart, this._undoText, ""));
				}
				this._undoStart = undefined;
				this._undoText = "";
			}
		},
		_onDestroy: function() {
			this.view.removeEventListener("ModelChanging", this, this._onModelChanging);
			this.view.removeEventListener("Destroy", this, this._onDestroy);
		},
		_onModelChanging: function(e) {
			var newText = e.text;
			var start = e.start;
			var removedCharCount = e.removedCharCount;
			var addedCharCount = e.addedCharCount;
			if (this._ignoreUndo) {
				return;
			}
			if (this._undoStart !== undefined && 
				!((addedCharCount === 1 && removedCharCount === 0 && start === this._undoStart + this._undoText.length) ||
					(addedCharCount === 0 && removedCharCount === 1 && (((start + 1) === -this._undoStart) || (start === -this._undoStart)))))
			{
				this._commitUndo();
			}
			if (!this.compoundChange) {
				if (addedCharCount === 1 && removedCharCount === 0) {
					if (this._undoStart === undefined) {
						this._undoStart = start;
					}
					this._undoText = this._undoText + newText;
					return;
				} else if (addedCharCount === 0 && removedCharCount === 1) {
					var deleting = this._undoText.length > 0 && -this._undoStart === start;
					this._undoStart = -start;
					if (deleting) {
						this._undoText = this._undoText + this.view.getText(start, start + removedCharCount);
					} else {
						this._undoText = this.view.getText(start, start + removedCharCount) + this._undoText;
					}
					return;
				}
			}
			this.add(new Change(start, newText, this.view.getText(start, start + removedCharCount)));
		}
	};
	return UndoStack;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.textview;
	});
}











 




 
var orion = orion || {};


 
orion.textview = orion.textview || {};


















orion.textview.TextModel = (function() {
	var isWindows = window.navigator.platform.indexOf("Win") !== -1;

	
	function TextModel(text, lineDelimiter) {
		this._listeners = [];
		this._lineDelimiter = lineDelimiter ? lineDelimiter : (isWindows ? "\r\n" : "\n"); 
		this._lastLineIndex = -1;
		this._text = [""];
		this._lineOffsets = [0];
		this.setText(text);
	}

	TextModel.prototype =  {
		








		addListener: function(listener) {
			this._listeners.push(listener);
		},
		






		removeListener: function(listener) {
			for (var i = 0; i < this._listeners.length; i++) {
				if (this._listeners[i] === listener) {
					this._listeners.splice(i, 1);
					return;
				}
			}
		},
		




		getCharCount: function() {
			var count = 0;
			for (var i = 0; i<this._text.length; i++) {
				count += this._text[i].length;
			}
			return count;
		},
		












		getLine: function(lineIndex, includeDelimiter) {
			var lineCount = this.getLineCount();
			if (!(0 <= lineIndex && lineIndex < lineCount)) {
				return null;
			}
			var start = this._lineOffsets[lineIndex];
			if (lineIndex + 1 < lineCount) {
				var text = this.getText(start, this._lineOffsets[lineIndex + 1]);
				if (includeDelimiter) {
					return text;
				}
				var end = text.length, c;
				while (((c = text.charCodeAt(end - 1)) === 10) || (c === 13)) {
					end--;
				}
				return text.substring(0, end);
			} else {
				return this.getText(start); 
			}
		},
		










		getLineAtOffset: function(offset) {
			if (!(0 <= offset && offset <= this.getCharCount())) {
				return -1;
			}
			var lineCount = this.getLineCount();
			var charCount = this.getCharCount();
			if (offset === charCount) {
				return lineCount - 1; 
			}
			var lineStart, lineEnd;
			var index = this._lastLineIndex;
			if (0 <= index && index < lineCount) {
				lineStart = this._lineOffsets[index];
				lineEnd = index + 1 < lineCount ? this._lineOffsets[index + 1] : charCount;
				if (lineStart <= offset && offset < lineEnd) {
					return index;
				}
			}
			var high = lineCount;
			var low = -1;
			while (high - low > 1) {
				index = Math.floor((high + low) / 2);
				lineStart = this._lineOffsets[index];
				lineEnd = index + 1 < lineCount ? this._lineOffsets[index + 1] : charCount;
				if (offset <= lineStart) {
					high = index;
				} else if (offset < lineEnd) {
					high = index;
					break;
				} else {
					low = index;
				}
			}
			this._lastLineIndex = high;
			return high;
		},
		







		getLineCount: function() {
			return this._lineOffsets.length;
		},
		






		getLineDelimiter: function() {
			return this._lineDelimiter;
		},
		

















		getLineEnd: function(lineIndex, includeDelimiter) {
			var lineCount = this.getLineCount();
			if (!(0 <= lineIndex && lineIndex < lineCount)) {
				return -1;
			}
			if (lineIndex + 1 < lineCount) {
				var end = this._lineOffsets[lineIndex + 1];
				if (includeDelimiter) {
					return end;
				}
				var text = this.getText(Math.max(this._lineOffsets[lineIndex], end - 2), end);
				var i = text.length, c;
				while (((c = text.charCodeAt(i - 1)) === 10) || (c === 13)) {
					i--;
				}
				return end - (text.length - i);
			} else {
				return this.getCharCount();
			}
		},
		











		getLineStart: function(lineIndex) {
			if (!(0 <= lineIndex && lineIndex < this.getLineCount())) {
				return -1;
			}
			return this._lineOffsets[lineIndex];
		},
		











		getText: function(start, end) {
			if (start === undefined) { start = 0; }
			if (end === undefined) { end = this.getCharCount(); }
			var offset = 0, chunk = 0, length;
			while (chunk<this._text.length) {
				length = this._text[chunk].length; 
				if (start <= offset + length) { break; }
				offset += length;
				chunk++;
			}
			var firstOffset = offset;
			var firstChunk = chunk;
			while (chunk<this._text.length) {
				length = this._text[chunk].length; 
				if (end <= offset + length) { break; }
				offset += length;
				chunk++;
			}
			var lastOffset = offset;
			var lastChunk = chunk;
			if (firstChunk === lastChunk) {
				return this._text[firstChunk].substring(start - firstOffset, end - lastOffset);
			}
			var beforeText = this._text[firstChunk].substring(start - firstOffset);
			var afterText = this._text[lastChunk].substring(0, end - lastOffset);
			return beforeText + this._text.slice(firstChunk+1, lastChunk).join("") + afterText; 
		},
		


















		onChanging: function(text, start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
			for (var i = 0; i < this._listeners.length; i++) {
				var l = this._listeners[i]; 
				if (l && l.onChanging) { 
					l.onChanging(text, start, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
				}
			}
		},
		

















		onChanged: function(start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
			for (var i = 0; i < this._listeners.length; i++) {
				var l = this._listeners[i]; 
				if (l && l.onChanged) { 
					l.onChanged(start, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
				}
			}
		},
		

















		setText: function(text, start, end) {
			if (text === undefined) { text = ""; }
			if (start === undefined) { start = 0; }
			if (end === undefined) { end = this.getCharCount(); }
			var startLine = this.getLineAtOffset(start);
			var endLine = this.getLineAtOffset(end);
			var eventStart = start;
			var removedCharCount = end - start;
			var removedLineCount = endLine - startLine;
			var addedCharCount = text.length;
			var addedLineCount = 0;
			var lineCount = this.getLineCount();
			
			var cr = 0, lf = 0, index = 0;
			var newLineOffsets = [];
			while (true) {
				if (cr !== -1 && cr <= index) { cr = text.indexOf("\r", index); }
				if (lf !== -1 && lf <= index) { lf = text.indexOf("\n", index); }
				if (lf === -1 && cr === -1) { break; }
				if (cr !== -1 && lf !== -1) {
					if (cr + 1 === lf) {
						index = lf + 1;
					} else {
						index = (cr < lf ? cr : lf) + 1;
					}
				} else if (cr !== -1) {
					index = cr + 1;
				} else {
					index = lf + 1;
				}
				newLineOffsets.push(start + index);
				addedLineCount++;
			}
		
			this.onChanging(text, eventStart, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
			
			
			if (newLineOffsets.length === 0) {
				var startLineOffset = this.getLineStart(startLine), endLineOffset;
				if (endLine + 1 < lineCount) {
					endLineOffset = this.getLineStart(endLine + 1);
				} else {
					endLineOffset = this.getCharCount();
				}
				if (start !== startLineOffset) {
					text = this.getText(startLineOffset, start) + text;
					start = startLineOffset;
				}
				if (end !== endLineOffset) {
					text = text + this.getText(end, endLineOffset);
					end = endLineOffset;
				}
			}
			
			var changeCount = addedCharCount - removedCharCount;
			for (var j = startLine + removedLineCount + 1; j < lineCount; j++) {
				this._lineOffsets[j] += changeCount;
			}
			var args = [startLine + 1, removedLineCount].concat(newLineOffsets);
			Array.prototype.splice.apply(this._lineOffsets, args);
			
			var offset = 0, chunk = 0, length;
			while (chunk<this._text.length) {
				length = this._text[chunk].length; 
				if (start <= offset + length) { break; }
				offset += length;
				chunk++;
			}
			var firstOffset = offset;
			var firstChunk = chunk;
			while (chunk<this._text.length) {
				length = this._text[chunk].length; 
				if (end <= offset + length) { break; }
				offset += length;
				chunk++;
			}
			var lastOffset = offset;
			var lastChunk = chunk;
			var firstText = this._text[firstChunk];
			var lastText = this._text[lastChunk];
			var beforeText = firstText.substring(0, start - firstOffset);
			var afterText = lastText.substring(end - lastOffset);
			var params = [firstChunk, lastChunk - firstChunk + 1];
			if (beforeText) { params.push(beforeText); }
			if (text) { params.push(text); }
			if (afterText) { params.push(afterText); }
			Array.prototype.splice.apply(this._text, params);
			if (this._text.length === 0) { this._text = [""]; }
			
			this.onChanged(eventStart, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
		}
	};
	
	return TextModel;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.textview;
	});
}

















 
var orion = orion || {};


 
orion.textview = orion.textview || {};















orion.textview.TextView = (function() {

	
	function addHandler(node, type, handler, capture) {
		if (typeof node.addEventListener === "function") {
			node.addEventListener(type, handler, capture === true);
		} else {
			node.attachEvent("on" + type, handler);
		}
	}
	
	function removeHandler(node, type, handler, capture) {
		if (typeof node.removeEventListener === "function") {
			node.removeEventListener(type, handler, capture === true);
		} else {
			node.detachEvent("on" + type, handler);
		}
	}
	var isIE = document.selection && window.ActiveXObject && /MSIE/.test(navigator.userAgent) ? document.documentMode : undefined;
	var isFirefox = parseFloat(navigator.userAgent.split("Firefox/")[1] || navigator.userAgent.split("Minefield/")[1]) || undefined;
	var isOpera = navigator.userAgent.indexOf("Opera") !== -1;
	var isChrome = navigator.userAgent.indexOf("Chrome") !== -1;
	var isSafari = navigator.userAgent.indexOf("Safari") !== -1;
	var isWebkit = navigator.userAgent.indexOf("WebKit") !== -1;
	var isPad = navigator.userAgent.indexOf("iPad") !== -1;
	var isMac = navigator.platform.indexOf("Mac") !== -1;
	var isWindows = navigator.platform.indexOf("Win") !== -1;
	var isLinux = navigator.platform.indexOf("Linux") !== -1;
	var isW3CEvents = typeof window.document.documentElement.addEventListener === "function";
	var isRangeRects = (!isIE || isIE >= 9) && typeof window.document.createRange().getBoundingClientRect === "function";
	var platformDelimiter = isWindows ? "\r\n" : "\n";
	
	





	var Selection = (function() {
		
		function Selection (start, end, caret) {
			




			this.start = start;
			




			this.end = end;
			
			this.caret = caret; 
		}
		Selection.prototype =  {
			
			clone: function() {
				return new Selection(this.start, this.end, this.caret);
			},
			
			collapse: function() {
				if (this.caret) {
					this.end = this.start;
				} else {
					this.start = this.end;
				}
			},
			
			extend: function (offset) {
				if (this.caret) {
					this.start = offset;
				} else {
					this.end = offset;
				}
				if (this.start > this.end) {
					var tmp = this.start;
					this.start = this.end;
					this.end = tmp;
					this.caret = !this.caret;
				}
			},
			
			setCaret: function(offset) {
				this.start = offset;
				this.end = offset;
				this.caret = false;
			},
			
			getCaret: function() {
				return this.caret ? this.start : this.end;
			},
			
			toString: function() {
				return "start=" + this.start + " end=" + this.end + (this.caret ? " caret is at start" : " caret is at end");
			},
			
			isEmpty: function() {
				return this.start === this.end;
			},
			
			equals: function(object) {
				return this.caret === object.caret && this.start === object.start && this.end === object.end;
			}
		};
		return Selection;
	}());

	






	var EventTable = (function() {
		
		function EventTable(){
		    this._listeners = {};
		}
		EventTable.prototype =  {
			
			addEventListener: function(type, context, func, data) {
				if (!this._listeners[type]) {
					this._listeners[type] = [];
				}
				var listener = {
						context: context,
						func: func,
						data: data
				};
				this._listeners[type].push(listener);
			},
			
			sendEvent: function(type, event) {
				var listeners = this._listeners[type];
				if (listeners) {
					for (var i=0, len=listeners.length; i < len; i++){
						var l = listeners[i];
						if (l && l.context && l.func) {
							l.func.call(l.context, event, l.data);
						}
					}
				}
			},
			
			removeEventListener: function(type, context, func, data){
				var listeners = this._listeners[type];
				if (listeners) {
					for (var i=0, len=listeners.length; i < len; i++){
						var l = listeners[i];
						if (l.context === context && l.func === func && l.data === data) {
							listeners.splice(i, 1);
							break;
						}
					}
				}
			}
		};
		return EventTable;
	}());
	
	
	function TextView (options) {
		this._init(options);
	}
	
	TextView.prototype =  {
		




















		addEventListener: function(type, context, func, data) {
			this._eventTable.addEventListener(type, context, func, data);
		},
		




		addRuler: function (ruler) {
			var document = this._frameDocument;
			var body = document.body;
			var side = ruler.getLocation();
			var rulerParent = side === "left" ? this._leftDiv : this._rightDiv;
			if (!rulerParent) {
				rulerParent = document.createElement("DIV");
				rulerParent.style.overflow = "hidden";
				rulerParent.style.MozUserSelect = "none";
				rulerParent.style.WebkitUserSelect = "none";
				if (isIE) {
					rulerParent.attachEvent("onselectstart", function() {return false;});
				}
				rulerParent.style.position = "absolute";
				rulerParent.style.top = "0px";
				rulerParent.style.cursor = "default";
				body.appendChild(rulerParent);
				if (side === "left") {
					this._leftDiv = rulerParent;
					rulerParent.className = "viewLeftRuler";
				} else {
					this._rightDiv = rulerParent;
					rulerParent.className = "viewRightRuler";
				}
				var table = document.createElement("TABLE");
				rulerParent.appendChild(table);
				table.cellPadding = "0px";
				table.cellSpacing = "0px";
				table.border = "0px";
				table.insertRow(0);
				var self = this;
				addHandler(rulerParent, "click", function(e) { self._handleRulerEvent(e); });
				addHandler(rulerParent, "dblclick", function(e) { self._handleRulerEvent(e); });
			}
			var div = document.createElement("DIV");
			div._ruler = ruler;
			div.rulerChanged = true;
			div.style.position = "relative";
			var row = rulerParent.firstChild.rows[0];
			var index = row.cells.length;
			var cell = row.insertCell(index);
			cell.vAlign = "top";
			cell.appendChild(div);
			ruler.setView(this);
			this._updatePage();
		},
		























		convert: function(rect, from, to) {
			var scroll = this._getScroll();
			var viewPad = this._getViewPadding();
			var frame = this._frame.getBoundingClientRect();
			var viewRect = this._viewDiv.getBoundingClientRect();
			switch(from) {
				case "document":
					if (rect.x !== undefined) {
						rect.x += - scroll.x + viewRect.left + viewPad.left;
					}
					if (rect.y !== undefined) {
						rect.y += - scroll.y + viewRect.top + viewPad.top;
					}
					break;
				case "page":
					if (rect.x !== undefined) {
						rect.x += - frame.left;
					}
					if (rect.y !== undefined) {
						rect.y += - frame.top;
					}
					break;
			}
			
			switch (to) {
				case "document":
					if (rect.x !== undefined) {
						rect.x += scroll.x - viewRect.left - viewPad.left;
					}
					if (rect.y !== undefined) {
						rect.y += scroll.y - viewRect.top - viewPad.top;
					}
					break;
				case "page":
					if (rect.x !== undefined) {
						rect.x += frame.left;
					}
					if (rect.y !== undefined) {
						rect.y += frame.top;
					}
					break;
			}
		},
		









		destroy: function() {
			this._setGrab(null);
			this._unhookEvents();
			
			
			var destroyRulers = function(rulerDiv) {
				if (!rulerDiv) {
					return;
				}
				var cells = rulerDiv.firstChild.rows[0].cells;
				for (var i = 0; i < cells.length; i++) {
					var div = cells[i].firstChild;
					div._ruler.setView(null);
				}
			};
			destroyRulers (this._leftDiv);
			destroyRulers (this._rightDiv);

			
			if (this._autoScrollTimerID) {
				clearTimeout(this._autoScrollTimerID);
				this._autoScrollTimerID = null;
			}
			if (this._updateTimer) {
				clearTimeout(this._updateTimer);
				this._updateTimer = null;
			}
			
			
			var parent = this._parent;
			var frame = this._frame;
			parent.removeChild(frame);
			
			if (isPad) {
				parent.removeChild(this._touchDiv);
				this._touchDiv = null;
				this._selDiv1 = null;
				this._selDiv2 = null;
				this._selDiv3 = null;
				this._textArea = null;
			}
			
			var e = {};
			this.onDestroy(e);
			
			this._parent = null;
			this._parentDocument = null;
			this._model = null;
			this._selection = null;
			this._doubleClickSelection = null;
			this._eventTable = null;
			this._frame = null;
			this._frameDocument = null;
			this._frameWindow = null;
			this._scrollDiv = null;
			this._viewDiv = null;
			this._clientDiv = null;
			this._overlayDiv = null;
			this._keyBindings = null;
			this._actions = null;
		},
		


		focus: function() {
			






			this._updateDOMSelection();
			if (isPad) {
				this._textArea.focus();
			} else {
				if (isOpera) { this._clientDiv.blur(); }
				this._clientDiv.focus();
			}
			



			this._updateDOMSelection();
		},
		

































































		getActions: function (defaultAction) {
			var result = [];
			var actions = this._actions;
			for (var i = 0; i < actions.length; i++) {
				if (!defaultAction && actions[i].defaultHandler) { continue; }
				result.push(actions[i].name);
			}
			return result;
		},
		














		getBottomIndex: function(fullyVisible) {
			return this._getBottomIndex(fullyVisible);
		},
		













		getBottomPixel: function() {
			return this._getScroll().y + this._getClientHeight();
		},
		








		getCaretOffset: function () {
			var s = this._getSelection();
			return s.getCaret();
		},
		













		getClientArea: function() {
			var scroll = this._getScroll();
			return {x: scroll.x, y: scroll.y, width: this._getClientWidth(), height: this._getClientHeight()};
		},
		












		getHorizontalPixel: function() {
			return this._getScroll().x;
		},
		








		getKeyBindings: function (name) {
			var result = [];
			var keyBindings = this._keyBindings;
			for (var i = 0; i < keyBindings.length; i++) {
				if (keyBindings[i].name === name) {
					result.push(keyBindings[i].keyBinding);
				}
			}
			return result;
		},
		








		getLineHeight: function(lineIndex) {
			return this._getLineHeight();
		},
		












		getLinePixel: function(lineIndex) {
			lineIndex = Math.min(Math.max(0, lineIndex), this._model.getLineCount());
			var lineHeight = this._getLineHeight();
			return lineHeight * lineIndex;
		},
		













		getLocationAtOffset: function(offset) {
			var model = this._model;
			offset = Math.min(Math.max(0, offset), model.getCharCount());
			var lineIndex = model.getLineAtOffset(offset);
			var scroll = this._getScroll();
			var viewRect = this._viewDiv.getBoundingClientRect();
			var viewPad = this._getViewPadding();
			var x = this._getOffsetToX(offset) + scroll.x - viewRect.left - viewPad.left;
			var y = this.getLinePixel(lineIndex);
			return {x: x, y: y};
		},
		




		getModel: function() {
			return this._model;
		},
		









		getOffsetAtLocation: function(x, y) {
			var model = this._model;
			var scroll = this._getScroll();
			var viewRect = this._viewDiv.getBoundingClientRect();
			var viewPad = this._getViewPadding();
			var lineIndex = this._getYToLine(y - scroll.y);
			x += -scroll.x + viewRect.left + viewPad.left;
			var offset = this._getXToOffset(lineIndex, x);
			return offset;
		},
		










		getSelection: function () {
			var s = this._getSelection();
			return {start: s.start, end: s.end};
		},
		










		getText: function(start, end) {
			var model = this._model;
			return model.getText(start, end);
		},
		














		getTopIndex: function(fullyVisible) {
			return this._getTopIndex(fullyVisible);
		},
		













		getTopPixel: function() {
			return this._getScroll().y;
		},
		

















		invokeAction: function (name, defaultAction) {
			var actions = this._actions;
			for (var i = 0; i < actions.length; i++) {
				var a = actions[i];
				if (a.name && a.name === name) {
					if (!defaultAction && a.userHandler) {
						if (a.userHandler()) { return; }
					}
					if (a.defaultHandler) { return a.defaultHandler(); }
					return false;
				}
			}
			return false;
		},
		













 
		




 
		onContextMenu: function(contextMenuEvent) { 
		  this._eventTable.sendEvent("ContextMenu", contextMenuEvent); 
		}, 
		








		







		onDestroy: function(destroyEvent) {
			this._eventTable.sendEvent("Destroy", destroyEvent);
		},
		











		












		














		





		onLineStyle: function(lineStyleEvent) {
			this._eventTable.sendEvent("LineStyle", lineStyleEvent);
		},
		















		





		onModelChanged: function(modelChangedEvent) {
			this._eventTable.sendEvent("ModelChanged", modelChangedEvent);
		},
		
















		





		onModelChanging: function(modelChangingEvent) {
			this._eventTable.sendEvent("ModelChanging", modelChangingEvent);
		},
		








		









		onModify: function(modifyEvent) {
			this._eventTable.sendEvent("Modify", modifyEvent);
		},
		











		





		onSelection: function(selectionEvent) {
			this._eventTable.sendEvent("Selection", selectionEvent);
		},
		











		





		onScroll: function(scrollEvent) {
			this._eventTable.sendEvent("Scroll", scrollEvent);
		},
		












		













		onVerify: function(verifyEvent) {
			this._eventTable.sendEvent("Verify", verifyEvent);
		},
		








		redrawLines: function(startLine, endLine, ruler) {
			if (startLine === undefined) { startLine = 0; }
			if (endLine === undefined) { endLine = this._model.getLineCount(); }
			if (startLine === endLine) { return; }
			var div = this._clientDiv;
			if (ruler) {
				var location = ruler.getLocation();
				var divRuler = location === "left" ? this._leftDiv : this._rightDiv;
				var cells = divRuler.firstChild.rows[0].cells;
				for (var i = 0; i < cells.length; i++) {
					if (cells[i].firstChild._ruler === ruler) {
						div = cells[i].firstChild;
						break;
					}
				}
			}
			if (ruler) {
				div.rulerChanged = true;
			}
			if (!ruler || ruler.getOverview() === "page") {
				var child = div.firstChild;
				while (child) {
					var lineIndex = child.lineIndex;
					if (startLine <= lineIndex && lineIndex < endLine) {
						child.lineChanged = true;
					}
					child = child.nextSibling;
				}
			}
			if (!ruler) {
				if (startLine <= this._maxLineIndex && this._maxLineIndex < endLine) {
					this._maxLineIndex = -1;
					this._maxLineWidth = 0;
				}
			}
			this._queueUpdatePage();
		},
		








		redrawRange: function(start, end) {
			var model = this._model;
			if (start === undefined) { start = 0; }
			if (end === undefined) { end = model.getCharCount(); }
			if (start === end) { return; }
			var startLine = model.getLineAtOffset(start);
			var endLine = model.getLineAtOffset(Math.max(0, end - 1)) + 1;
			this.redrawLines(startLine, endLine);
		},
		












		removeEventListener: function(type, context, func, data) {
			this._eventTable.removeEventListener(type, context, func, data);
		},
		




		removeRuler: function (ruler) {
			ruler.setView(null);
			var side = ruler.getLocation();
			var rulerParent = side === "left" ? this._leftDiv : this._rightDiv;
			var row = rulerParent.firstChild.rows[0];
			var cells = row.cells;
			for (var index = 0; index < cells.length; index++) {
				var cell = cells[index];
				if (cell.firstChild._ruler === ruler) { break; }
			}
			if (index === cells.length) { return; }
			row.cells[index]._ruler = undefined;
			row.deleteCell(index);
			this._updatePage();
		},
		













		setAction: function(name, handler) {
			if (!name) { return; }
			var actions = this._actions;
			for (var i = 0; i < actions.length; i++) {
				var a = actions[i];
				if (a.name === name) {
					a.userHandler = handler;
					return;
				}
			}
			actions.push({name: name, userHandler: handler});
		},
		







		setKeyBinding: function(keyBinding, name) {
			var keyBindings = this._keyBindings;
			for (var i = 0; i < keyBindings.length; i++) {
				var kb = keyBindings[i]; 
				if (kb.keyBinding.equals(keyBinding)) {
					if (name) {
						kb.name = name;
					} else {
						if (kb.predefined) {
							kb.name = null;
						} else {
							var oldName = kb.name; 
							keyBindings.splice(i, 1);
							var index = 0;
							while (index < keyBindings.length && oldName !== keyBindings[index].name) {
								index++;
							}
							if (index === keyBindings.length) {
								





								var actions = this._actions;
								for (var j = 0; j < actions.length; j++) {
									if (actions[j].name === oldName) {
										if (!actions[j].defaultHandler) {
											actions.splice(j, 1);
										}
									}
								}
							}
						}
					}
					return;
				}
			}
			if (name) {
				keyBindings.push({keyBinding: keyBinding, name: name});
			}
		},
		









		setCaretOffset: function(offset, show) {
			var charCount = this._model.getCharCount();
			offset = Math.max(0, Math.min (offset, charCount));
			var selection = new Selection(offset, offset, false);
			this._setSelection (selection, show === undefined || show);
		},
		












		setHorizontalPixel: function(pixel) {
			pixel = Math.max(0, pixel);
			this._scrollView(pixel - this._getScroll().x, 0);
		},
		




		setModel: function(model) {
			if (!model) { return; }
			this._model.removeListener(this._modelListener);
			var oldLineCount = this._model.getLineCount();
			var oldCharCount = this._model.getCharCount();
			var newLineCount = model.getLineCount();
			var newCharCount = model.getCharCount();
			var newText = model.getText();
			var e = {
				text: newText,
				start: 0,
				removedCharCount: oldCharCount,
				addedCharCount: newCharCount,
				removedLineCount: oldLineCount,
				addedLineCount: newLineCount
			};
			this.onModelChanging(e); 
			this.redrawRange();
			this._model = model;
			e = {
				start: 0,
				removedCharCount: oldCharCount,
				addedCharCount: newCharCount,
				removedLineCount: oldLineCount,
				addedLineCount: newLineCount
			};
			this.onModelChanged(e); 
			this._model.addListener(this._modelListener);
			this.redrawRange();
		},
		




















		setSelection: function (start, end, show) {
			var caret = start > end;
			if (caret) {
				var tmp = start;
				start = end;
				end = tmp;
			}
			var charCount = this._model.getCharCount();
			start = Math.max(0, Math.min (start, charCount));
			end = Math.max(0, Math.min (end, charCount));
			var selection = new Selection(start, end, caret);
			this._setSelection(selection, show === undefined || show);
		},
		
















		setText: function (text, start, end) {
			var reset = start === undefined && end === undefined;
			if (start === undefined) { start = 0; }
			if (end === undefined) { end = this._model.getCharCount(); }
			this._modifyContent({text: text, start: start, end: end, _code: true}, !reset);
			if (reset) {
				this._columnX = -1;
				this._setSelection(new Selection (0, 0, false), true);
				
				




				if (isFirefox) {
					var hasFocus = this._hasFocus;
					var clientDiv = this._clientDiv;
					if (hasFocus) { clientDiv.blur(); }
					clientDiv.contentEditable = false;
					clientDiv.contentEditable = true;
					if (hasFocus) { clientDiv.focus(); }
				}
			}
		},
		











		setTopIndex: function(topIndex) {
			var model = this._model;
			if (model.getCharCount() === 0) {
				return;
			}
			var lineCount = model.getLineCount();
			var lineHeight = this._getLineHeight();
			var pageSize = Math.max(1, Math.min(lineCount, Math.floor(this._getClientHeight () / lineHeight)));
			if (topIndex < 0) {
				topIndex = 0;
			} else if (topIndex > lineCount - pageSize) {
				topIndex = lineCount - pageSize;
			}
			var pixel = topIndex * lineHeight - this._getScroll().y;
			this._scrollView(0, pixel);
		},
		













		setTopPixel: function(pixel) {
			var lineHeight = this._getLineHeight();
			var clientHeight = this._getClientHeight();
			var lineCount = this._model.getLineCount();
			pixel = Math.min(Math.max(0, pixel), lineHeight * lineCount - clientHeight);
			this._scrollView(0, pixel - this._getScroll().y);
		},
		





		showSelection: function() {
			return this._showCaret();
		},
		
		
		_handleBodyMouseDown: function (e) {
			if (!e) { e = window.event; }
			






			var topNode = isOpera ? this._clientDiv : this._overlayDiv || this._viewDiv;
			
			var temp = e.target ? e.target : e.srcElement;
			while (temp) {
				if (topNode === temp) {
					return;
				}
				temp = temp.parentNode;
			}
			if (e.preventDefault) { e.preventDefault(); }
			if (e.stopPropagation){ e.stopPropagation(); }
			if (!isW3CEvents) {
				


 
				topNode.setCapture();
				setTimeout(function() { topNode.releaseCapture(); }, 0);
			}
		},
		_handleBlur: function (e) {
			if (!e) { e = window.event; }
			this._hasFocus = false;
			




			if (isIE < 9) {
				if (!this._getSelection().isEmpty()) {
					var document = this._frameDocument;
					var child = document.createElement("DIV");
					var body = document.body;
					body.appendChild(child);
					body.removeChild(child);
				}
			}
			if (isFirefox || isIE) {
				if (this._selDiv1) {
					var color = isIE ? "transparent" : "#AFAFAF";
					this._selDiv1.style.background = color;
					this._selDiv2.style.background = color;
					this._selDiv3.style.background = color;
				}
			}
		},
		_handleContextMenu: function (e) {
			if (!e) { e = window.event; }
			var scroll = this._getScroll(); 
			var viewRect = this._viewDiv.getBoundingClientRect(); 
			var viewPad = this._getViewPadding(); 
			var x = e.clientX + scroll.x - viewRect.left - viewPad.left; 
			var y = e.clientY + scroll.y - viewRect.top - viewPad.top; 
			this.onContextMenu({x: x, y: y, screenX: e.screenX, screenY: e.screenY}); 
			if (e.preventDefault) { e.preventDefault(); }
			return false;
		},
		_handleCopy: function (e) {
			if (this._ignoreCopy) { return; }
			if (!e) { e = window.event; }
			if (this._doCopy(e)) {
				if (e.preventDefault) { e.preventDefault(); }
				return false;
			}
		},
		_handleCut: function (e) {
			if (!e) { e = window.event; }
			if (this._doCut(e)) {
				if (e.preventDefault) { e.preventDefault(); }
				return false;
			}
		},
		_handleDataModified: function(e) {
			this._startIME();
		},
		_handleDblclick: function (e) {
			if (!e) { e = window.event; }
			var time = e.timeStamp ? e.timeStamp : new Date().getTime();
			this._lastMouseTime = time;
			if (this._clickCount !== 2) {
				this._clickCount = 2;
				this._handleMouse(e);
			}
		},
		_handleDragStart: function (e) {
			if (!e) { e = window.event; }
			if (e.preventDefault) { e.preventDefault(); }
			return false;
		},
		_handleDragOver: function (e) {
			if (!e) { e = window.event; }
			e.dataTransfer.dropEffect = "none";
			if (e.preventDefault) { e.preventDefault(); }
			return false;
		},
		_handleDrop: function (e) {
			if (!e) { e = window.event; }
			if (e.preventDefault) { e.preventDefault(); }
			return false;
		},
		_handleDocFocus: function (e) {
			if (!e) { e = window.event; }
			this._clientDiv.focus();
		},
		_handleFocus: function (e) {
			if (!e) { e = window.event; }
			this._hasFocus = true;
			





			if (isIE) {
				this._updateDOMSelection();
			}
			if (isFirefox || isIE) {
				if (this._selDiv1) {
					var color = this._hightlightRGB;
					this._selDiv1.style.background = color;
					this._selDiv2.style.background = color;
					this._selDiv3.style.background = color;
				}
			}
		},
		_handleKeyDown: function (e) {
			if (!e) { e = window.event; }
			if (isPad) {
				if (e.keyCode === 8) {
					this._doBackspace({});
					e.preventDefault();
				}
				return;
			}
			if (e.keyCode === 229) {
				if (this.readonly) {
					if (e.preventDefault) { e.preventDefault(); }
					return false;
				}
				this._startIME();
			} else {
				this._commitIME();
			}
			










			if (((isMac || isLinux) && isFirefox < 4) || isOpera) {
				this._keyDownEvent = e;
				return true;
			}
			
			if (this._doAction(e)) {
				if (e.preventDefault) {
					e.preventDefault(); 
				} else {
					e.cancelBubble = true;
					e.returnValue = false;
					e.keyCode = 0;
				}
				return false;
			}
		},
		_handleKeyPress: function (e) {
			if (!e) { e = window.event; }
			




			if (isMac && isWebkit) {
				if ((0xF700 <= e.keyCode && e.keyCode <= 0xF7FF) || e.keyCode === 13 || e.keyCode === 8) {
					if (e.preventDefault) { e.preventDefault(); }
					return false;
				}
			}
			if (((isMac || isLinux) && isFirefox < 4) || isOpera) {
				if (this._doAction(this._keyDownEvent)) {
					if (e.preventDefault) { e.preventDefault(); }
					return false;
				}
			}
			var ctrlKey = isMac ? e.metaKey : e.ctrlKey;
			if (e.charCode !== undefined) {
				if (ctrlKey) {
					switch (e.charCode) {
						




						case 99:
						case 118:
						case 120:
							return true;
					}
				}
			}
			var ignore = false;
			if (isMac) {
				if (e.ctrlKey || e.metaKey) { ignore = true; }
			} else {
				if (isFirefox) {
					
					if (e.ctrlKey || e.altKey) { ignore = true; }
				} else {
					
					if (e.ctrlKey ^ e.altKey) { ignore = true; }
				}
			}
			if (!ignore) {
				var key = isOpera ? e.which : (e.charCode !== undefined ? e.charCode : e.keyCode);
				if (key > 31) {
					this._doContent(String.fromCharCode (key));
					if (e.preventDefault) { e.preventDefault(); }
					return false;
				}
			}
		},
		_handleKeyUp: function (e) {
			if (!e) { e = window.event; }
			
			
			if (e.keyCode === 13) {
				this._commitIME();
			}
		},
		_handleMouse: function (e) {
			var target = this._frameWindow;
			if (isIE) { target = this._clientDiv; }
			if (this._overlayDiv) {
				var self = this;
				setTimeout(function () {
					self.focus();
				}, 0);
			}
			if (this._clickCount === 1) {
				this._setGrab(target);
				this._setSelectionTo(e.clientX, e.clientY, e.shiftKey);
			} else {
				












				if (isW3CEvents) { this._setGrab(target); }
				
				this._doubleClickSelection = null;
				this._setSelectionTo(e.clientX, e.clientY, e.shiftKey);
				this._doubleClickSelection = this._getSelection();
			}
		},
		_handleMouseDown: function (e) {
			if (!e) { e = window.event; }
			var left = e.which ? e.button === 0 : e.button === 1;
			this._commitIME();
			if (left) {
				this._isMouseDown = true;
				var deltaX = Math.abs(this._lastMouseX - e.clientX);
				var deltaY = Math.abs(this._lastMouseY - e.clientY);
				var time = e.timeStamp ? e.timeStamp : new Date().getTime();  
				if ((time - this._lastMouseTime) <= this._clickTime && deltaX <= this._clickDist && deltaY <= this._clickDist) {
					this._clickCount++;
				} else {
					this._clickCount = 1;
				}
				this._lastMouseX = e.clientX;
				this._lastMouseY = e.clientY;
				this._lastMouseTime = time;
				this._handleMouse(e);
				if (isOpera) {
						if (!this._hasFocus) {
							this.focus();
						}
						e.preventDefault();
				}
			}
		},
		_handleMouseMove: function (e) {
			if (!e) { e = window.event; }
			
















			if (!isW3CEvents) {
				if (e.button === 0) {
					this._setGrab(null);
					return true;
				}
				if (!this._isMouseDown && e.button === 1 && (this._clickCount & 1) !== 0) {
					this._clickCount = 2;
					return this._handleMouse(e, this._clickCount);
				}
			}
			
			var x = e.clientX;
			var y = e.clientY;
			var viewPad = this._getViewPadding();
			var viewRect = this._viewDiv.getBoundingClientRect();
			var width = this._getClientWidth (), height = this._getClientHeight();
			var leftEdge = viewRect.left + viewPad.left;
			var topEdge = viewRect.top + viewPad.top;
			var rightEdge = viewRect.left + viewPad.left + width;
			var bottomEdge = viewRect.top + viewPad.top + height;
			var model = this._model;
			var caretLine = model.getLineAtOffset(this._getSelection().getCaret());
			if (y < topEdge && caretLine !== 0) {
				this._doAutoScroll("up", x, y - topEdge);
			} else if (y > bottomEdge && caretLine !== model.getLineCount() - 1) {
				this._doAutoScroll("down", x, y - bottomEdge);
			} else if (x < leftEdge) {
				this._doAutoScroll("left", x - leftEdge, y);
			} else if (x > rightEdge) {
				this._doAutoScroll("right", x - rightEdge, y);
			} else {
				this._endAutoScroll();
				this._setSelectionTo(x, y, true);
				






				if (isIE) {
					var body = this._frameDocument.body;
					body.getBoundingClientRect();
				}
			}
		},
		_handleMouseUp: function (e) {
			if (!e) { e = window.event; }
			this._endAutoScroll();
			var left = e.which ? e.button === 0 : e.button === 1;
			if (left) {
				this._isMouseDown=false;
				
				












				if (isW3CEvents) { this._setGrab(null); }
			}
		},
		_handleMouseWheel: function (e) {
			if (!e) { e = window.event; }
			var lineHeight = this._getLineHeight();
			var pixelX = 0, pixelY = 0;
			
			if (isFirefox) {
				var pixel;
				if (isMac) {
					pixel = e.detail * 3;
				} else {
					var limit = 256;
					pixel = Math.max(-limit, Math.min(limit, e.detail)) * lineHeight;
				}
				if (e.axis === e.HORIZONTAL_AXIS) {
					pixelX = pixel;
				} else {
					pixelY = pixel;
				}
			} else {
				
				if (isMac) {
					












					var denominatorX = 40, denominatorY = 40;
					if (isChrome) {
						if (e.wheelDeltaX % 120 !== 0) { denominatorX = 1; }
						if (e.wheelDeltaY % 120 !== 0) { denominatorY = 1; }
					}
					pixelX = -e.wheelDeltaX / denominatorX;
					if (-1 < pixelX && pixelX < 0) { pixelX = -1; }
					if (0 < pixelX && pixelX < 1) { pixelX = 1; }
					pixelY = -e.wheelDeltaY / denominatorY;
					if (-1 < pixelY && pixelY < 0) { pixelY = -1; }
					if (0 < pixelY && pixelY < 1) { pixelY = 1; }
				} else {
					pixelX = -e.wheelDeltaX;
					var linesToScroll = 8;
					pixelY = (-e.wheelDeltaY / 120 * linesToScroll) * lineHeight;
				}
			}
			







			if (isSafari) {
				var lineDiv = e.target;
				while (lineDiv && lineDiv.lineIndex === undefined) {
					lineDiv = lineDiv.parentNode;
				}
				this._mouseWheelLine = lineDiv;
			}
			var oldScroll = this._getScroll();
			this._scrollView(pixelX, pixelY);
			var newScroll = this._getScroll();
			if (isSafari) { this._mouseWheelLine = null; }
			if (oldScroll.x !== newScroll.x || oldScroll.y !== newScroll.y) {
				if (e.preventDefault) { e.preventDefault(); }
				return false;
			}
		},
		_handlePaste: function (e) {
			if (this._ignorePaste) { return; }
			if (!e) { e = window.event; }
			if (this._doPaste(e)) {
				if (isIE) {
					


					var self = this;
					setTimeout(function() {self._updateDOMSelection();}, 0);
				}
				if (e.preventDefault) { e.preventDefault(); }
				return false;
			}
		},
		_handleResize: function (e) {
			if (!e) { e = window.event; }
			var element = this._frameDocument.documentElement;
			var newWidth = element.clientWidth;
			var newHeight = element.clientHeight;
			if (this._frameWidth !== newWidth || this._frameHeight !== newHeight) {
				this._frameWidth = newWidth;
				this._frameHeight = newHeight;
				this._updatePage();
			}
		},
		_handleRulerEvent: function (e) {
			if (!e) { e = window.event; }
			var target = e.target ? e.target : e.srcElement;
			var lineIndex = target.lineIndex;
			var element = target;
			while (element && !element._ruler) {
				if (lineIndex === undefined && element.lineIndex !== undefined) {
					lineIndex = element.lineIndex;
				}
				element = element.parentNode;
			}
			var ruler = element ? element._ruler : null;
			if (isPad && lineIndex === undefined && ruler && ruler.getOverview() === "document") {
				var buttonHeight = 17;
				var clientHeight = this._getClientHeight ();
				var lineHeight = this._getLineHeight ();
				var viewPad = this._getViewPadding();
				var trackHeight = clientHeight + viewPad.top + viewPad.bottom - 2 * buttonHeight;
				var pixels = this._model.getLineCount () * lineHeight;
				this.setTopPixel(Math.floor((e.clientY - buttonHeight - lineHeight) * pixels / trackHeight));
			}
			if (ruler) {
				switch (e.type) {
					case "click":
						if (ruler.onClick) { ruler.onClick(lineIndex, e); }
						break;
					case "dblclick": 
						if (ruler.onDblClick) { ruler.onDblClick(lineIndex, e); }
						break;
				}
			}
		},
		_handleScroll: function () {
			this._doScroll(this._getScroll());
		},
		_handleSelectStart: function (e) {
			if (!e) { e = window.event; }
			if (this._ignoreSelect) {
				if (e && e.preventDefault) { e.preventDefault(); }
				return false;
			}
		},
		_handleInput: function (e) {
			var textArea = this._textArea;
			this._doContent(textArea.value);
			textArea.selectionStart = textArea.selectionEnd = 0;
			textArea.value = "";
			e.preventDefault();
		},
		_handleTextInput: function (e) {
			this._doContent(e.data);
			e.preventDefault();
		},
		_touchConvert: function (touch) {
			var rect = this._frame.getBoundingClientRect();
			var body = this._parentDocument.body;
			return {left: touch.clientX - rect.left - body.scrollLeft, top: touch.clientY - rect.top - body.scrollTop};
		},
		_handleTouchStart: function (e) {
			var touches = e.touches, touch, pt, sel;
			this._touchMoved = false;
			this._touchStartScroll = undefined;
			if (touches.length === 1) {
				touch = touches[0];
				var pageX = touch.pageX;
				var pageY = touch.pageY;
				this._touchStartX = pageX;
				this._touchStartY = pageY;
				this._touchStartTime = e.timeStamp;
				this._touchStartScroll = this._getScroll();
				sel = this._getSelection();
				pt = this._touchConvert(touches[0]);
				this._touchGesture = "none";
				if (!sel.isEmpty()) {
					if (this._hitOffset(sel.end, pt.left, pt.top)) {
						this._touchGesture = "extendEnd";
					} else if (this._hitOffset(sel.start, pt.left, pt.top)) {
						this._touchGesture = "extendStart";
					}
				}
				if (this._touchGesture === "none") {
					var textArea = this._textArea;
					textArea.value = "";
					textArea.style.left = "-1000px";
					textArea.style.top = "-1000px";
					textArea.style.width = "3000px";
					textArea.style.height = "3000px";
					var self = this;
					
					var f = function() {
						self._touchTimeout = null;
						self._clickCount = 1;
						self._setSelectionTo(pt.left, pt.top, false);
					};
					this._touchTimeout = setTimeout(f, 200);
				}
			} else if (touches.length === 2) {
				this._touchGesture = "select";
				if (this._touchTimeout) {
					clearTimeout(this._touchTimeout);
					this._touchTimeout = null;
				}
				pt = this._touchConvert(touches[0]);
				var offset1 = this._getXToOffset(this._getYToLine(pt.top), pt.left);
				pt = this._touchConvert(touches[1]);
				var offset2 = this._getXToOffset(this._getYToLine(pt.top), pt.left);
				sel = this._getSelection();
				sel.setCaret(offset1);
				sel.extend(offset2);
				this._setSelection(sel, true, true);
			}
			

		},
		_handleTouchMove: function (e) {
			this._touchMoved = true;
			var touches = e.touches, pt, sel;
			if (touches.length === 1) {
				var touch = touches[0];
				var pageX = touch.pageX;
				var pageY = touch.pageY;
				var deltaX = this._touchStartX - pageX;
				var deltaY = this._touchStartY - pageY;
				pt = this._touchConvert(touch);
				sel = this._getSelection();
				if (this._touchTimeout) {
					clearTimeout(this._touchTimeout);
					this._touchTimeout = null;
				}
				if (this._touchGesture === "none") {
					if ((e.timeStamp - this._touchStartTime) < 200 && (Math.abs(deltaX) > 5 || Math.abs(deltaY) > 5)) {
						this._touchGesture = "scroll";
					} else {
						this._touchGesture = "caret";
					}
				}
				if (this._touchGesture === "select") {
					if (this._hitOffset(sel.end, pt.left, pt.top)) {
						this._touchGesture = "extendEnd";
					} else if (this._hitOffset(sel.start, pt.left, pt.top)) {
						this._touchGesture = "extendStart";
					} else {
						this._touchGesture = "caret";
					}
				}
				switch (this._touchGesture) {
					case "scroll":
						this._touchStartX = pageX;
						this._touchStartY = pageY;
						this._scrollView(deltaX, deltaY);
						break;
					case "extendStart":
					case "extendEnd":
						this._clickCount = 1;
						var lineIndex = this._getYToLine(pt.top);
						var offset = this._getXToOffset(lineIndex, pt.left);
						sel.setCaret(this._touchGesture === "extendStart" ? sel.end : sel.start);
						sel.extend(offset);
						if (offset >= sel.end && this._touchGesture === "extendStart") {
							this._touchGesture = "extendEnd";
						}
						if (offset <= sel.start && this._touchGesture === "extendEnd") {
							this._touchGesture = "extendStart";
						}
						this._setSelection(sel, true, true);
						break;
					case "caret":
						this._setSelectionTo(pt.left, pt.top, false);
						break;
				}
			} else if (touches.length === 2) {
				pt = this._touchConvert(touches[0]);
				var offset1 = this._getXToOffset(this._getYToLine(pt.top), pt.left);
				pt = this._touchConvert(touches[1]);
				var offset2 = this._getXToOffset(this._getYToLine(pt.top), pt.left);
				sel = this._getSelection();
				sel.setCaret(offset1);
				sel.extend(offset2);
				this._setSelection(sel, true, true);
			}
			e.preventDefault();
		},
		_handleTouchEnd: function (e) {
			if (!this._touchMoved) {
				if (e.touches.length === 0 && e.changedTouches.length === 1 && this._touchTimeout) {
					clearTimeout(this._touchTimeout);
					this._touchTimeout = null;
					var touch = e.changedTouches[0];
					this._clickCount = 1;
					var pt = this._touchConvert(touch);
					this._setSelectionTo(pt.left, pt.top, false);
				}
			}
			if (e.touches.length === 0) {
				var self = this;
				setTimeout(function() {
					var selection = self._getSelection();
					var text = self._model.getText(selection.start, selection.end);
					var textArea = self._textArea;
					textArea.value = text;
					textArea.selectionStart = 0;
					textArea.selectionEnd = text.length;
					if (!selection.isEmpty()) {
						var touchRect = self._touchDiv.getBoundingClientRect();
						var bounds = self._getOffsetBounds(selection.start);
						textArea.style.left = (touchRect.width / 2) + "px";
						textArea.style.top = ((bounds.top > 40 ? bounds.top - 30 : bounds.top + 30)) + "px";
					}
				}, 0);
			}
			e.preventDefault();
		},

		
		_doAction: function (e) {
			var keyBindings = this._keyBindings;
			for (var i = 0; i < keyBindings.length; i++) {
				var kb = keyBindings[i];
				if (kb.keyBinding.match(e)) {
					if (kb.name) {
						var actions = this._actions;
						for (var j = 0; j < actions.length; j++) {
							var a = actions[j];
							if (a.name === kb.name) {
								if (a.userHandler) {
									if (!a.userHandler()) {
										if (a.defaultHandler) {
											a.defaultHandler();
										} else {
											return false;
										}
									}
								} else if (a.defaultHandler) {
									a.defaultHandler();
								}
								break;
							}
						}
					}
					return true;
				}
			}
			return false;
		},
		_doBackspace: function (args) {
			var selection = this._getSelection();
			if (selection.isEmpty()) {
				var model = this._model;
				var caret = selection.getCaret();
				var lineIndex = model.getLineAtOffset(caret);
				var lineStart = model.getLineStart(lineIndex);
				if (caret === lineStart) {
					if (lineIndex > 0) {
						selection.extend(model.getLineEnd(lineIndex - 1));
					}
				} else {
					var newOffset = args.toLineStart ? lineStart : this._getOffset(caret, args.unit, -1);
					selection.extend(newOffset);
				}
			}
			this._modifyContent({text: "", start: selection.start, end: selection.end}, true);
			return true;
		},
		_doContent: function (text) {
			var selection = this._getSelection();
			this._modifyContent({text: text, start: selection.start, end: selection.end, _ignoreDOMSelection: true}, true);
		},
		_doCopy: function (e) {
			var selection = this._getSelection();
			if (!selection.isEmpty()) {
				var text = this._model.getText(selection.start, selection.end);
				return this._setClipboardText(text, e);
			}
			return true;
		},
		_doCursorNext: function (args) {
			if (!args.select) {
				if (this._clearSelection("next")) { return true; }
			}
			var model = this._model;
			var selection = this._getSelection();
			var caret = selection.getCaret();
			var lineIndex = model.getLineAtOffset(caret);
			if (caret === model.getLineEnd(lineIndex)) {
				if (lineIndex + 1 < model.getLineCount()) {
					selection.extend(model.getLineStart(lineIndex + 1));
				}
			} else {
				selection.extend(this._getOffset(caret, args.unit, 1));
			}
			if (!args.select) { selection.collapse(); }
			this._setSelection(selection, true);
			return true;
		},
		_doCursorPrevious: function (args) {
			if (!args.select) {
				if (this._clearSelection("previous")) { return true; }
			}
			var model = this._model;
			var selection = this._getSelection();
			var caret = selection.getCaret();
			var lineIndex = model.getLineAtOffset(caret);
			if (caret === model.getLineStart(lineIndex)) {
				if (lineIndex > 0) {
					selection.extend(model.getLineEnd(lineIndex - 1));
				}
			} else {
				selection.extend(this._getOffset(caret, args.unit, -1));
			}
			if (!args.select) { selection.collapse(); }
			this._setSelection(selection, true);
			return true;
		},
		_doCut: function (e) {
			var selection = this._getSelection();
			if (!selection.isEmpty()) {
				var text = this._model.getText(selection.start, selection.end);
				this._doContent("");
				return this._setClipboardText(text, e);
			}
			return true;
		},
		_doDelete: function (args) {
			var selection = this._getSelection();
			if (selection.isEmpty()) {
				var model = this._model;
				var caret = selection.getCaret();
				var lineIndex = model.getLineAtOffset(caret);
				var lineEnd = model.getLineEnd(lineIndex);
				if (caret === lineEnd) {
					if (lineIndex + 1 < model.getLineCount()) {
						selection.extend(model.getLineStart(lineIndex + 1));
					}
				} else {
					var newOffset = args.toLineEnd ? lineEnd : this._getOffset(caret, args.unit, 1);
					selection.extend(newOffset);
				}
			}
			this._modifyContent({text: "", start: selection.start, end: selection.end}, true);
			return true;
		},
		_doEnd: function (args) {
			var selection = this._getSelection();
			var model = this._model;

			if (args.scrollOnly) {
				var lineCount = model.getLineCount();
				var clientHeight = this._getClientHeight();
				var lineHeight = this._getLineHeight();
				var verticalMaximum = lineCount * lineHeight;
				var currentScrollOffset = this._getScroll().y;
				var scrollOffset = verticalMaximum - clientHeight;
				if (scrollOffset > currentScrollOffset) {
					this._scrollView(0, scrollOffset - currentScrollOffset);
				}
				return true;
			}

			if (args.ctrl) {
				selection.extend(model.getCharCount());
			} else {
				var lineIndex = model.getLineAtOffset(selection.getCaret());
				selection.extend(model.getLineEnd(lineIndex)); 
			}
			if (!args.select) { selection.collapse(); }
			this._setSelection(selection, true);
			return true;
		},
		_doEnter: function (args) {
			var model = this._model;
			this._doContent(model.getLineDelimiter()); 
			return true;
		},
		_doHome: function (args) {
			if (args.scrollOnly) {
				var currentScrollOffset = this._getScroll().y;
				if (currentScrollOffset > 0) {
					this._scrollView(0, -currentScrollOffset);
				}
				return true;
			}

			var selection = this._getSelection();
			var model = this._model;
			if (args.ctrl) {
				selection.extend(0);
			} else {
				var lineIndex = model.getLineAtOffset(selection.getCaret());
				selection.extend(model.getLineStart(lineIndex)); 
			}
			if (!args.select) { selection.collapse(); }
			this._setSelection(selection, true);
			return true;
		},
		_doLineDown: function (args) {
			var model = this._model;
			var selection = this._getSelection();
			var caret = selection.getCaret();
			var lineIndex = model.getLineAtOffset(caret);
			if (lineIndex + 1 < model.getLineCount()) {
				var x = this._columnX;
				if (x === -1 || args.select) {
					x = this._getOffsetToX(caret);
				}
				selection.extend(this._getXToOffset(lineIndex + 1, x));
				if (!args.select) { selection.collapse(); }
				this._setSelection(selection, true, true);
				this._columnX = x;
			}
			return true;
		},
		_doLineUp: function (args) {
			var model = this._model;
			var selection = this._getSelection();
			var caret = selection.getCaret();
			var lineIndex = model.getLineAtOffset(caret);
			if (lineIndex > 0) {
				var x = this._columnX;
				if (x === -1 || args.select) {
					x = this._getOffsetToX(caret);
				}
				selection.extend(this._getXToOffset(lineIndex - 1, x));
				if (!args.select) { selection.collapse(); }
				this._setSelection(selection, true, true);
				this._columnX = x;
			}
			return true;
		},
		_doPageDown: function (args) {
			var model = this._model;
			var selection, caret, caretLine;
			if (args.scrollOnly) {
				caretLine = this.getBottomIndex(true);
			} else {
				selection = this._getSelection();
				caret = selection.getCaret();
				caretLine = model.getLineAtOffset(caret);
			}
			var lineCount = model.getLineCount();
			if (caretLine < lineCount - 1) {
				var clientHeight = this._getClientHeight();
				var lineHeight = this._getLineHeight();
				var lines = Math.floor(clientHeight / lineHeight);
				var scrollLines = Math.min(lineCount - caretLine - 1, lines);
				scrollLines = Math.max(1, scrollLines);
				var x = this._columnX;
				if (!args.scrollOnly) {
					if (x === -1 || args.select) {
						x = this._getOffsetToX(caret);
					}
					selection.extend(this._getXToOffset(caretLine + scrollLines, x));
					if (!args.select) { selection.collapse(); }
					this._setSelection(selection, false, false);
				}

				var verticalMaximum = lineCount * lineHeight;
				var verticalScrollOffset = this._getScroll().y;
				var scrollOffset = verticalScrollOffset + scrollLines * lineHeight;
				if (scrollOffset + clientHeight > verticalMaximum) {
					scrollOffset = verticalMaximum - clientHeight;
				} 
				if (scrollOffset > verticalScrollOffset) {
					this._scrollView(0, scrollOffset - verticalScrollOffset);
				} else if (!args.scrollOnly) {
					this._updateDOMSelection();
				}
				this._columnX = x;
			}
			return true;
		},
		_doPageUp: function (args) {
			var model = this._model;
			var selection, caret, caretLine;
			if (args.scrollOnly) {
				caretLine = this.getTopIndex(true);
			} else {
				selection = this._getSelection();
				caret = selection.getCaret();
				caretLine = model.getLineAtOffset(caret);
			}

			if (caretLine > 0) {
				var clientHeight = this._getClientHeight();
				var lineHeight = this._getLineHeight();
				var lines = Math.floor(clientHeight / lineHeight);
				var scrollLines = Math.max(1, Math.min(caretLine, lines));
				var x = this._columnX;
				if (!args.scrollOnly) {
					if (x === -1 || args.select) {
						x = this._getOffsetToX(caret);
					}
					selection.extend(this._getXToOffset(caretLine - scrollLines, x));
					if (!args.select) { selection.collapse(); }
					this._setSelection(selection, false, false);
				}

				var verticalScrollOffset = this._getScroll().y;
				var scrollOffset = Math.max(0, verticalScrollOffset - scrollLines * lineHeight);
				if (scrollOffset < verticalScrollOffset) {
					this._scrollView(0, scrollOffset - verticalScrollOffset);
				} else if (!args.scrollOnly) {
					this._updateDOMSelection();
				}
				this._columnX = x;
			}
			return true;
		},
		_doPaste: function(e) {
			var text = this._getClipboardText(e);
			if (text) {
				this._doContent(text);
			}
			return text !== null;
		},
		_doScroll: function (scroll) {
			var oldX = this._hScroll;
			var oldY = this._vScroll;
			if (oldX !== scroll.x || oldY !== scroll.y) {
				this._hScroll = scroll.x;
				this._vScroll = scroll.y;
				this._commitIME();
				this._updatePage();
				var e = {
					oldValue: {x: oldX, y: oldY},
					newValue: scroll
				};
				this.onScroll(e);
			}
		},
		_doSelectAll: function (args) {
			var model = this._model;
			var selection = this._getSelection();
			selection.setCaret(0);
			selection.extend(model.getCharCount());
			this._setSelection(selection, false);
			return true;
		},
		_doTab: function (args) {
			this._doContent("\t"); 
			return true;
		},
		
		
		_applyStyle: function(style, node) {
			if (!style) {
				return;
			}
			if (style.styleClass) {
				node.className = style.styleClass;
			}
			var properties = style.style;
			if (properties) {
				for (var s in properties) {
					if (properties.hasOwnProperty(s)) {
						node.style[s] = properties[s];
					}
				}
			}
		},
		_autoScroll: function () {
			var selection = this._getSelection();
			var line;
			var x = this._autoScrollX;
			if (this._autoScrollDir === "up" || this._autoScrollDir === "down") {
				var scroll = this._autoScrollY / this._getLineHeight();
				scroll = scroll < 0 ? Math.floor(scroll) : Math.ceil(scroll);
				line = this._model.getLineAtOffset(selection.getCaret());
				line = Math.max(0, Math.min(this._model.getLineCount() - 1, line + scroll));
			} else if (this._autoScrollDir === "left" || this._autoScrollDir === "right") {
				line = this._getYToLine(this._autoScrollY);
				x += this._getOffsetToX(selection.getCaret());
			}
			selection.extend(this._getXToOffset(line, x));
			this._setSelection(selection, true);
		},
		_autoScrollTimer: function () {
			this._autoScroll();
			var self = this;
			this._autoScrollTimerID = setTimeout(function () {self._autoScrollTimer();}, this._AUTO_SCROLL_RATE);
		},
		_calculateLineHeight: function() {
			var parent = this._clientDiv;
			var document = this._frameDocument;
			var c = " ";
			var line = document.createElement("DIV");
			line.style.position = "fixed";
			line.style.left = "-1000px";
			var span1 = document.createElement("SPAN");
			span1.appendChild(document.createTextNode(c));
			line.appendChild(span1);
			var span2 = document.createElement("SPAN");
			span2.style.fontStyle = "italic";
			span2.appendChild(document.createTextNode(c));
			line.appendChild(span2);
			var span3 = document.createElement("SPAN");
			span3.style.fontWeight = "bold";
			span3.appendChild(document.createTextNode(c));
			line.appendChild(span3);
			var span4 = document.createElement("SPAN");
			span4.style.fontWeight = "bold";
			span4.style.fontStyle = "italic";
			span4.appendChild(document.createTextNode(c));
			line.appendChild(span4);
			parent.appendChild(line);
			var spanRect1 = span1.getBoundingClientRect();
			var spanRect2 = span2.getBoundingClientRect();
			var spanRect3 = span3.getBoundingClientRect();
			var spanRect4 = span4.getBoundingClientRect();
			var h1 = spanRect1.bottom - spanRect1.top;
			var h2 = spanRect2.bottom - spanRect2.top;
			var h3 = spanRect3.bottom - spanRect3.top;
			var h4 = spanRect4.bottom - spanRect4.top;
			var fontStyle = 0;
			var lineHeight = h1;
			if (h2 > h1) {
				lineHeight = h2;
				fontStyle = 1;
			}
			if (h3 > h2) {
				lineHeight = h3;
				fontStyle = 2;
			}
			if (h4 > h3) {
				lineHeight = h4;
				fontStyle = 3;
			}
			this._largestFontStyle = fontStyle;
			parent.removeChild(line);
			return lineHeight;
		},
		_calculatePadding: function() {
			var document = this._frameDocument;
			var parent = this._clientDiv;
			var pad = this._getPadding(this._viewDiv);
			var div1 = document.createElement("DIV");
			div1.style.position = "fixed";
			div1.style.left = "-1000px";
			div1.style.paddingLeft = pad.left + "px";
			div1.style.paddingTop = pad.top + "px";
			div1.style.paddingRight = pad.right + "px";
			div1.style.paddingBottom = pad.bottom + "px";
			div1.style.width = "100px";
			div1.style.height = "100px";
			var div2 = document.createElement("DIV");
			div2.style.width = "100%";
			div2.style.height = "100%";
			div1.appendChild(div2);
			parent.appendChild(div1);
			var rect1 = div1.getBoundingClientRect();
			var rect2 = div2.getBoundingClientRect();
			parent.removeChild(div1);
			pad = {
				left: rect2.left - rect1.left,
				top: rect2.top - rect1.top,
				right: rect1.right - rect2.right,
				bottom: rect1.bottom - rect2.bottom
			};
			return pad;
		},
		_clearSelection: function (direction) {
			var selection = this._getSelection();
			if (selection.isEmpty()) { return false; }
			if (direction === "next") {
				selection.start = selection.end;
			} else {
				selection.end = selection.start;
			}
			this._setSelection(selection, true);
			return true;
		},
		_commitIME: function () {
			if (this._imeOffset === -1) { return; }
			
			
			
			this._scrollDiv.focus();
			this._clientDiv.focus();
			
			var model = this._model;
			var lineIndex = model.getLineAtOffset(this._imeOffset);
			var lineStart = model.getLineStart(lineIndex);
			var newText = this._getDOMText(lineIndex);
			var oldText = model.getLine(lineIndex);
			var start = this._imeOffset - lineStart;
			var end = start + newText.length - oldText.length;
			if (start !== end) {
				var insertText = newText.substring(start, end);
				this._doContent(insertText);
			}
			this._imeOffset = -1;
		},
		_convertDelimiter: function (text, addTextFunc, addDelimiterFunc) {
				var cr = 0, lf = 0, index = 0, length = text.length;
				while (index < length) {
					if (cr !== -1 && cr <= index) { cr = text.indexOf("\r", index); }
					if (lf !== -1 && lf <= index) { lf = text.indexOf("\n", index); }
					var start = index, end;
					if (lf === -1 && cr === -1) {
						addTextFunc(text.substring(index));
						break;
					}
					if (cr !== -1 && lf !== -1) {
						if (cr + 1 === lf) {
							end = cr;
							index = lf + 1;
						} else {
							end = cr < lf ? cr : lf;
							index = (cr < lf ? cr : lf) + 1;
						}
					} else if (cr !== -1) {
						end = cr;
						index = cr + 1;
					} else {
						end = lf;
						index = lf + 1;
					}
					addTextFunc(text.substring(start, end));
					addDelimiterFunc();
				}
		},
		_createActions: function () {
			var KeyBinding = orion.textview.KeyBinding;
			
			var bindings = this._keyBindings = [];

			
			bindings.push({name: "lineUp",		keyBinding: new KeyBinding(38), predefined: true});
			bindings.push({name: "lineDown",	keyBinding: new KeyBinding(40), predefined: true});
			bindings.push({name: "charPrevious",	keyBinding: new KeyBinding(37), predefined: true});
			bindings.push({name: "charNext",	keyBinding: new KeyBinding(39), predefined: true});
			if (isMac) {
				bindings.push({name: "scrollPageUp",		keyBinding: new KeyBinding(33), predefined: true});
				bindings.push({name: "scrollPageDown",	keyBinding: new KeyBinding(34), predefined: true});
				bindings.push({name: "pageUp",		keyBinding: new KeyBinding(33, null, null, true), predefined: true});
				bindings.push({name: "pageDown",	keyBinding: new KeyBinding(34, null, null, true), predefined: true});
				bindings.push({name: "lineStart",	keyBinding: new KeyBinding(37, true), predefined: true});
				bindings.push({name: "lineEnd",		keyBinding: new KeyBinding(39, true), predefined: true});
				bindings.push({name: "wordPrevious",	keyBinding: new KeyBinding(37, null, null, true), predefined: true});
				bindings.push({name: "wordNext",	keyBinding: new KeyBinding(39, null, null, true), predefined: true});
				bindings.push({name: "scrollTextStart",	keyBinding: new KeyBinding(36), predefined: true});
				bindings.push({name: "scrollTextEnd",		keyBinding: new KeyBinding(35), predefined: true});
				bindings.push({name: "textStart",	keyBinding: new KeyBinding(38, true), predefined: true});
				bindings.push({name: "textEnd",		keyBinding: new KeyBinding(40, true), predefined: true});
			} else {
				bindings.push({name: "pageUp",		keyBinding: new KeyBinding(33), predefined: true});
				bindings.push({name: "pageDown",	keyBinding: new KeyBinding(34), predefined: true});
				bindings.push({name: "lineStart",	keyBinding: new KeyBinding(36), predefined: true});
				bindings.push({name: "lineEnd",		keyBinding: new KeyBinding(35), predefined: true});
				bindings.push({name: "wordPrevious",	keyBinding: new KeyBinding(37, true), predefined: true});
				bindings.push({name: "wordNext",	keyBinding: new KeyBinding(39, true), predefined: true});
				bindings.push({name: "textStart",	keyBinding: new KeyBinding(36, true), predefined: true});
				bindings.push({name: "textEnd",		keyBinding: new KeyBinding(35, true), predefined: true});
			}

			
			bindings.push({name: "selectLineUp",		keyBinding: new KeyBinding(38, null, true), predefined: true});
			bindings.push({name: "selectLineDown",		keyBinding: new KeyBinding(40, null, true), predefined: true});
			bindings.push({name: "selectCharPrevious",	keyBinding: new KeyBinding(37, null, true), predefined: true});
			bindings.push({name: "selectCharNext",		keyBinding: new KeyBinding(39, null, true), predefined: true});
			bindings.push({name: "selectPageUp",		keyBinding: new KeyBinding(33, null, true), predefined: true});
			bindings.push({name: "selectPageDown",		keyBinding: new KeyBinding(34, null, true), predefined: true});
			if (isMac) {
				bindings.push({name: "selectLineStart",	keyBinding: new KeyBinding(37, true, true), predefined: true});
				bindings.push({name: "selectLineEnd",		keyBinding: new KeyBinding(39, true, true), predefined: true});
				bindings.push({name: "selectWordPrevious",	keyBinding: new KeyBinding(37, null, true, true), predefined: true});
				bindings.push({name: "selectWordNext",	keyBinding: new KeyBinding(39, null, true, true), predefined: true});
				bindings.push({name: "selectTextStart",	keyBinding: new KeyBinding(36, null, true), predefined: true});
				bindings.push({name: "selectTextEnd",		keyBinding: new KeyBinding(35, null, true), predefined: true});
				bindings.push({name: "selectTextStart",	keyBinding: new KeyBinding(38, true, true), predefined: true});
				bindings.push({name: "selectTextEnd",		keyBinding: new KeyBinding(40, true, true), predefined: true});
			} else {
				bindings.push({name: "selectLineStart",		keyBinding: new KeyBinding(36, null, true), predefined: true});
				bindings.push({name: "selectLineEnd",		keyBinding: new KeyBinding(35, null, true), predefined: true});
				bindings.push({name: "selectWordPrevious",	keyBinding: new KeyBinding(37, true, true), predefined: true});
				bindings.push({name: "selectWordNext",		keyBinding: new KeyBinding(39, true, true), predefined: true});
				bindings.push({name: "selectTextStart",		keyBinding: new KeyBinding(36, true, true), predefined: true});
				bindings.push({name: "selectTextEnd",		keyBinding: new KeyBinding(35, true, true), predefined: true});
			}

			
			bindings.push({name: "deletePrevious",		keyBinding: new KeyBinding(8), predefined: true});
			bindings.push({name: "deletePrevious",		keyBinding: new KeyBinding(8, null, true), predefined: true});
			bindings.push({name: "deleteNext",		keyBinding: new KeyBinding(46), predefined: true});
			bindings.push({name: "deleteWordPrevious",	keyBinding: new KeyBinding(8, true), predefined: true});
			bindings.push({name: "deleteWordPrevious",	keyBinding: new KeyBinding(8, true, true), predefined: true});
			bindings.push({name: "deleteWordNext",		keyBinding: new KeyBinding(46, true), predefined: true});
			bindings.push({name: "tab",			keyBinding: new KeyBinding(9), predefined: true});
			bindings.push({name: "enter",			keyBinding: new KeyBinding(13), predefined: true});
			bindings.push({name: "enter",			keyBinding: new KeyBinding(13, null, true), predefined: true});
			bindings.push({name: "selectAll",		keyBinding: new KeyBinding('a', true), predefined: true});
			if (isMac) {
				bindings.push({name: "deleteNext",		keyBinding: new KeyBinding(46, null, true), predefined: true});
				bindings.push({name: "deleteWordPrevious",	keyBinding: new KeyBinding(8, null, null, true), predefined: true});
				bindings.push({name: "deleteWordNext",		keyBinding: new KeyBinding(46, null, null, true), predefined: true});
			}
				
			




			if (!isFirefox) {
				var isMacChrome = isMac && isChrome;
				bindings.push({name: null, keyBinding: new KeyBinding('u', !isMacChrome, false, false, isMacChrome), predefined: true});
				bindings.push({name: null, keyBinding: new KeyBinding('i', !isMacChrome, false, false, isMacChrome), predefined: true});
				bindings.push({name: null, keyBinding: new KeyBinding('b', !isMacChrome, false, false, isMacChrome), predefined: true});
			}

			if (isFirefox) {
				bindings.push({name: "copy", keyBinding: new KeyBinding(45, true), predefined: true});
				bindings.push({name: "paste", keyBinding: new KeyBinding(45, null, true), predefined: true});
				bindings.push({name: "cut", keyBinding: new KeyBinding(46, null, true), predefined: true});
			}

			
			if (isMac) {
				bindings.push({name: "lineStart", keyBinding: new KeyBinding("a", false, false, false, true), predefined: true});
				bindings.push({name: "lineEnd", keyBinding: new KeyBinding("e", false, false, false, true), predefined: true});
				bindings.push({name: "lineUp", keyBinding: new KeyBinding("p", false, false, false, true), predefined: true});
				bindings.push({name: "lineDown", keyBinding: new KeyBinding("n", false, false, false, true), predefined: true});
				bindings.push({name: "charPrevious", keyBinding: new KeyBinding("b", false, false, false, true), predefined: true});
				bindings.push({name: "charNext", keyBinding: new KeyBinding("f", false, false, false, true), predefined: true});
				bindings.push({name: "deletePrevious", keyBinding: new KeyBinding("h", false, false, false, true), predefined: true});
				bindings.push({name: "deleteNext", keyBinding: new KeyBinding("d", false, false, false, true), predefined: true});
				bindings.push({name: "deleteLineEnd", keyBinding: new KeyBinding("k", false, false, false, true), predefined: true});
				if (isFirefox) {
					bindings.push({name: "scrollPageDown", keyBinding: new KeyBinding("v", false, false, false, true), predefined: true});
					bindings.push({name: "deleteLineStart", keyBinding: new KeyBinding("u", false, false, false, true), predefined: true});
					bindings.push({name: "deleteWordPrevious", keyBinding: new KeyBinding("w", false, false, false, true), predefined: true});
				} else {
					bindings.push({name: "pageDown", keyBinding: new KeyBinding("v", false, false, false, true), predefined: true});
					
				}
			}

			
			var self = this;
			this._actions = [
				{name: "lineUp",		defaultHandler: function() {return self._doLineUp({select: false});}},
				{name: "lineDown",		defaultHandler: function() {return self._doLineDown({select: false});}},
				{name: "lineStart",		defaultHandler: function() {return self._doHome({select: false, ctrl:false});}},
				{name: "lineEnd",		defaultHandler: function() {return self._doEnd({select: false, ctrl:false});}},
				{name: "charPrevious",		defaultHandler: function() {return self._doCursorPrevious({select: false, unit:"character"});}},
				{name: "charNext",		defaultHandler: function() {return self._doCursorNext({select: false, unit:"character"});}},
				{name: "pageUp",		defaultHandler: function() {return self._doPageUp({select: false});}},
				{name: "pageDown",		defaultHandler: function() {return self._doPageDown({select: false});}},
				{name: "scrollPageUp",		defaultHandler: function() {return self._doPageUp({scrollOnly: true});}},
				{name: "scrollPageDown",		defaultHandler: function() {return self._doPageDown({scrollOnly: true});}},
				{name: "wordPrevious",		defaultHandler: function() {return self._doCursorPrevious({select: false, unit:"word"});}},
				{name: "wordNext",		defaultHandler: function() {return self._doCursorNext({select: false, unit:"word"});}},
				{name: "textStart",		defaultHandler: function() {return self._doHome({select: false, ctrl:true});}},
				{name: "textEnd",		defaultHandler: function() {return self._doEnd({select: false, ctrl:true});}},
				{name: "scrollTextStart",	defaultHandler: function() {return self._doHome({scrollOnly: true});}},
				{name: "scrollTextEnd",		defaultHandler: function() {return self._doEnd({scrollOnly: true});}},
				
				{name: "selectLineUp",		defaultHandler: function() {return self._doLineUp({select: true});}},
				{name: "selectLineDown",	defaultHandler: function() {return self._doLineDown({select: true});}},
				{name: "selectLineStart",	defaultHandler: function() {return self._doHome({select: true, ctrl:false});}},
				{name: "selectLineEnd",		defaultHandler: function() {return self._doEnd({select: true, ctrl:false});}},
				{name: "selectCharPrevious",	defaultHandler: function() {return self._doCursorPrevious({select: true, unit:"character"});}},
				{name: "selectCharNext",	defaultHandler: function() {return self._doCursorNext({select: true, unit:"character"});}},
				{name: "selectPageUp",		defaultHandler: function() {return self._doPageUp({select: true});}},
				{name: "selectPageDown",	defaultHandler: function() {return self._doPageDown({select: true});}},
				{name: "selectWordPrevious",	defaultHandler: function() {return self._doCursorPrevious({select: true, unit:"word"});}},
				{name: "selectWordNext",	defaultHandler: function() {return self._doCursorNext({select: true, unit:"word"});}},
				{name: "selectTextStart",	defaultHandler: function() {return self._doHome({select: true, ctrl:true});}},
				{name: "selectTextEnd",		defaultHandler: function() {return self._doEnd({select: true, ctrl:true});}},

				{name: "deletePrevious",	defaultHandler: function() {return self._doBackspace({unit:"character"});}},
				{name: "deleteNext",		defaultHandler: function() {return self._doDelete({unit:"character"});}},
				{name: "deleteWordPrevious",	defaultHandler: function() {return self._doBackspace({unit:"word"});}},
				{name: "deleteWordNext",	defaultHandler: function() {return self._doDelete({unit:"word"});}},
				{name: "deleteLineStart",	defaultHandler: function() {return self._doBackspace({toLineStart: true});}},
				{name: "deleteLineEnd",	defaultHandler: function() {return self._doDelete({toLineEnd: true});}},
				{name: "tab",			defaultHandler: function() {return self._doTab();}},
				{name: "enter",			defaultHandler: function() {return self._doEnter();}},
				{name: "selectAll",		defaultHandler: function() {return self._doSelectAll();}},
				{name: "copy",			defaultHandler: function() {return self._doCopy();}},
				{name: "cut",			defaultHandler: function() {return self._doCut();}},
				{name: "paste",			defaultHandler: function() {return self._doPaste();}}
			];
		},
		_createLine: function(parent, sibling, document, lineIndex, model) {
			var lineText = model.getLine(lineIndex);
			var lineStart = model.getLineStart(lineIndex);
			var e = {lineIndex: lineIndex, lineText: lineText, lineStart: lineStart};
			this.onLineStyle(e);
			var child = document.createElement("DIV");
			child.lineIndex = lineIndex;
			this._applyStyle(e.style, child);
			if (lineText.length !== 0) {
				var start = 0;
				var tabSize = this._tabSize;
				if (tabSize && tabSize !== 8) {
					var tabIndex = lineText.indexOf("\t"), ignoreChars = 0;
					while (tabIndex !== -1) {
						this._createRange(child, document, e.ranges, start, tabIndex, lineText, lineStart);
						var spacesCount = tabSize - ((tabIndex + ignoreChars) % tabSize);
						var spaces = "\u00A0";
						for (var i = 1; i < spacesCount; i++) {
							spaces += " ";
						}
						var tabSpan = document.createElement("SPAN");
						tabSpan.appendChild(document.createTextNode(spaces));
						tabSpan.ignoreChars = spacesCount - 1;
						ignoreChars += tabSpan.ignoreChars;
						if (e.ranges) {
							for (var j = 0; j < e.ranges.length; j++) {
								var range = e.ranges[j];
								var styleStart = range.start - lineStart;
								var styleEnd = range.end - lineStart;
								if (styleStart > tabIndex) { break; } 
								if (styleStart <= tabIndex && tabIndex < styleEnd) {
									this._applyStyle(range.style, tabSpan);
									break;
								}
							}
						} 
						child.appendChild(tabSpan);
						start = tabIndex + 1;
						tabIndex = lineText.indexOf("\t", start);
					}
				}
				this._createRange(child, document, e.ranges, start, lineText.length, lineText, lineStart);
			}
			
			







			var span = document.createElement("SPAN");
			span.ignoreChars = 1;
			if ((this._largestFontStyle & 1) !== 0) {
				span.style.fontStyle = "italic";
			}
			if ((this._largestFontStyle & 2) !== 0) {
				span.style.fontWeight = "bold";
			}
			var c = " ";
			if (!this._fullSelection && isIE < 9) {
				




				c = "\uFEFF";
			}
			if (isWebkit) {
				






				c = "\u200C";
			}
			span.appendChild(document.createTextNode(c));
			child.appendChild(span);
			
			parent.insertBefore(child, sibling);
			return child;
		},
		_createRange: function(parent, document, ranges, start, end, text, lineStart) {
			if (start >= end) { return; }
			var span;
			if (ranges) {
				for (var i = 0; i < ranges.length; i++) {
					var range = ranges[i];
					if (range.end <= lineStart + start) { continue; }
					var styleStart = Math.max(lineStart + start, range.start) - lineStart;
					if (styleStart >= end) { break; }
					var styleEnd = Math.min(lineStart + end, range.end) - lineStart;
					if (styleStart < styleEnd) {
						styleStart = Math.max(start, styleStart);
						styleEnd = Math.min(end, styleEnd);
						if (start < styleStart) {
							span = document.createElement("SPAN");
							span.appendChild(document.createTextNode(text.substring(start, styleStart)));
							parent.appendChild(span);
						}
						span = document.createElement("SPAN");
						span.appendChild(document.createTextNode(text.substring(styleStart, styleEnd)));
						this._applyStyle(range.style, span);
						parent.appendChild(span);
						start = styleEnd;
					}
				}
			}
			if (start < end) {
				span = document.createElement("SPAN");
				span.appendChild(document.createTextNode(text.substring(start, end)));
				parent.appendChild(span);
			}
		},
		_doAutoScroll: function (direction, x, y) {
			this._autoScrollDir = direction;
			this._autoScrollX = x;
			this._autoScrollY = y;
			if (!this._autoScrollTimerID) {
				this._autoScrollTimer();
			}
		},
		_endAutoScroll: function () {
			if (this._autoScrollTimerID) { clearTimeout(this._autoScrollTimerID); }
			this._autoScrollDir = undefined;
			this._autoScrollTimerID = undefined;
		},
		_getBoundsAtOffset: function (offset) {
			var model = this._model;
			var document = this._frameDocument;
			var clientDiv = this._clientDiv;
			var lineIndex = model.getLineAtOffset(offset);
			var dummy;
			var child = this._getLineNode(lineIndex);
			if (!child) {
				child = dummy = this._createLine(clientDiv, null, document, lineIndex, model);
			}
			var result = null;
			if (offset < model.getLineEnd(lineIndex)) {
				var lineOffset = model.getLineStart(lineIndex);
				var lineChild = child.firstChild;
				while (lineChild) {
					var textNode = lineChild.firstChild;
					var nodeLength = textNode.length; 
					if (lineChild.ignoreChars) {
						nodeLength -= lineChild.ignoreChars;
					}
					if (lineOffset + nodeLength > offset) {
						var index = offset - lineOffset;
						var range;
						if (isRangeRects) {
							range = document.createRange();
							range.setStart(textNode, index);
							range.setEnd(textNode, index + 1);
							result = range.getBoundingClientRect();
						} else if (isIE) {
							range = document.body.createTextRange();
							range.moveToElementText(lineChild);
							range.collapse();
							range.moveEnd("character", index + 1);
							range.moveStart("character", index);
							result = range.getBoundingClientRect();
						} else {
							var text = textNode.data;
							lineChild.removeChild(textNode);
							lineChild.appendChild(document.createTextNode(text.substring(0, index)));
							var span = document.createElement("SPAN");
							span.appendChild(document.createTextNode(text.substring(index, index + 1)));
							lineChild.appendChild(span);
							lineChild.appendChild(document.createTextNode(text.substring(index + 1)));
							result = span.getBoundingClientRect();
							lineChild.innerHTML = "";
							lineChild.appendChild(textNode);
							if (!dummy) {
								




								var s = this._getSelection();
								if ((lineOffset <= s.start && s.start < lineOffset + nodeLength) ||  (lineOffset <= s.end && s.end < lineOffset + nodeLength)) {
									this._updateDOMSelection();
								}
							}
						}
						if (isIE) {
							var logicalXDPI = window.screen.logicalXDPI;
							var deviceXDPI = window.screen.deviceXDPI;
							result.left = result.left * logicalXDPI / deviceXDPI;
							result.right = result.right * logicalXDPI / deviceXDPI;
						}
						break;
					}
					lineOffset += nodeLength;
					lineChild = lineChild.nextSibling;
				}
			}
			if (!result) {
				var rect = this._getLineBoundingClientRect(child);
				result = {left: rect.right, right: rect.right};
			}
			if (dummy) { clientDiv.removeChild(dummy); }
			return result;
		},
		_getBottomIndex: function (fullyVisible) {
			var child = this._bottomChild;
			if (fullyVisible && this._getClientHeight() > this._getLineHeight()) {
				var rect = child.getBoundingClientRect();
				var clientRect = this._clientDiv.getBoundingClientRect();
				if (rect.bottom > clientRect.bottom) {
					child = this._getLinePrevious(child) || child;
				}
			}
			return child.lineIndex;
		},
		_getFrameHeight: function() {
			return this._frameDocument.documentElement.clientHeight;
		},
		_getFrameWidth: function() {
			return this._frameDocument.documentElement.clientWidth;
		},
		_getClientHeight: function() {
			var viewPad = this._getViewPadding();
			return Math.max(0, this._viewDiv.clientHeight - viewPad.top - viewPad.bottom);
		},
		_getClientWidth: function() {
			var viewPad = this._getViewPadding();
			return Math.max(0, this._viewDiv.clientWidth - viewPad.left - viewPad.right);
		},
		_getClipboardText: function (event) {
			var delimiter = this._model.getLineDelimiter();
			var clipboadText, text;
			if (this._frameWindow.clipboardData) {
				
				clipboadText = [];
				text = this._frameWindow.clipboardData.getData("Text");
				this._convertDelimiter(text, function(t) {clipboadText.push(t);}, function() {clipboadText.push(delimiter);});
				return clipboadText.join("");
			}
			if (isFirefox) {
				var window = this._frameWindow;
				var document = this._frameDocument;
				var child = document.createElement("PRE");
				child.style.position = "fixed";
				child.style.left = "-1000px";
				child.appendChild(document.createTextNode(" "));
				this._clientDiv.appendChild(child);
				var range = document.createRange();
				range.selectNodeContents(child);
				var sel = window.getSelection();
				if (sel.rangeCount > 0) { sel.removeAllRanges(); }
				sel.addRange(range);
				var self = this;
				
				var cleanup = function() {
					self._updateDOMSelection();
					




					if (child.parent === self._clientDiv) {
						self._clientDiv.removeChild(child);
					}
				};
				var _getText = function() {
					



					var endNode = null;
					if (sel.anchorNode.nodeType !== child.TEXT_NODE) {
						endNode = sel.anchorNode.childNodes[sel.anchorOffset];
					}
					var text = [];
					
					var getNodeText = function(node) {
						var nodeChild = node.firstChild;
						while (nodeChild && nodeChild !== endNode) {
							if (nodeChild.nodeType === child.TEXT_NODE) {
								text.push(nodeChild !== sel.anchorNode ? nodeChild.data : nodeChild.data.substring(0, sel.anchorOffset));
							} else if (nodeChild.tagName === "BR") {
								text.push(delimiter); 
							} else {
								getNodeText(nodeChild);
							}
							nodeChild = nodeChild.nextSibling;
						}
					};
					getNodeText(child);
					cleanup();
					return text.join("");
				};
				
				
				var result = false;
				this._ignorePaste = true;
				try {
					result = document.execCommand("paste", false, null);
				} catch (ex) {}
				this._ignorePaste = false;
				if (!result) {
					


					if (event) {
						setTimeout(function() {
							var text = _getText();
							if (text) { self._doContent(text); }
						}, 0);
						return null;
					} else {
						
						cleanup();
						return "";
					}
				}
				return _getText();
			}
			
			if (event && event.clipboardData) {
				



				clipboadText = [];
				text = event.clipboardData.getData("text/plain");
				this._convertDelimiter(text, function(t) {clipboadText.push(t);}, function() {clipboadText.push(delimiter);});
				return clipboadText.join("");
			} else {
				
			}
			return "";
		},
		_getDOMText: function(lineIndex) {
			var child = this._getLineNode(lineIndex);
			var lineChild = child.firstChild;
			var text = "";
			while (lineChild) {
				var textNode = lineChild.firstChild;
				while (textNode) {
					if (lineChild.ignoreChars) {
						for (var i = 0; i < textNode.length; i++) {
							var ch = textNode.data.substring(i, i + 1);
							if (ch !== " ") {
								text += ch;
							}
						}
					} else {
						text += textNode.data;
					}
					textNode = textNode.nextSibling;
				}
				lineChild = lineChild.nextSibling;
			}
			return text;
		},
		_getViewPadding: function() {
			return this._viewPadding;
		},
		_getLineBoundingClientRect: function (child) {
			var rect = child.getBoundingClientRect();
			var lastChild = child.lastChild;
			
			while (lastChild && lastChild.ignoreChars === lastChild.firstChild.length) {
				lastChild = lastChild.previousSibling;
			}
			if (!lastChild) {
				return {left: rect.left, top: rect.top, right: rect.left, bottom: rect.bottom};
			}
			var lastRect = lastChild.getBoundingClientRect();
			return {left: rect.left, top: rect.top, right: lastRect.right, bottom: rect.bottom};
		},
		_getLineHeight: function() {
			return this._lineHeight;
		},
		_getLineNode: function (lineIndex) {
			var clientDiv = this._clientDiv;
			var child = clientDiv.firstChild;
			while (child) {
				if (lineIndex === child.lineIndex) {
					return child;
				}
				child = child.nextSibling;
			}
			return undefined;
		},
		_getLineNext: function (lineNode) {
			var node = lineNode ? lineNode.nextSibling : this._clientDiv.firstChild;
			while (node && node.lineIndex === -1) {
				node = node.nextSibling;
			}
			return node;
		},
		_getLinePrevious: function (lineNode) {
			var node = lineNode ? lineNode.previousSibling : this._clientDiv.lastChild;
			while (node && node.lineIndex === -1) {
				node = node.previousSibling;
			}
			return node;
		},
		_getOffset: function (offset, unit, direction) {
			if (unit === "wordend") {
				return this._getOffset_W3C(offset, unit, direction);
			}
			return isIE ? this._getOffset_IE(offset, unit, direction) : this._getOffset_W3C(offset, unit, direction);
		},
		_getOffset_W3C: function (offset, unit, direction) {
			function _isPunctuation(c) {
				return (33 <= c && c <= 47) || (58 <= c && c <= 64) || (91 <= c && c <= 94) || c === 96 || (123 <= c && c <= 126);
			}
			function _isWhitespace(c) {
				return c === 32 || c === 9;
			}
			if (unit === "word" || unit === "wordend") {
				var model = this._model;
				var lineIndex = model.getLineAtOffset(offset);
				var lineText = model.getLine(lineIndex);
				var lineStart = model.getLineStart(lineIndex);
				var lineEnd = model.getLineEnd(lineIndex);
				var lineLength = lineText.length;
				var offsetInLine = offset - lineStart;
				
				
				var c, previousPunctuation, previousLetterOrDigit, punctuation, letterOrDigit;
				if (direction > 0) {
					if (offsetInLine === lineLength) { return lineEnd; }
					c = lineText.charCodeAt(offsetInLine);
					previousPunctuation = _isPunctuation(c); 
					previousLetterOrDigit = !previousPunctuation && !_isWhitespace(c);
					offsetInLine++;
					while (offsetInLine < lineLength) {
						c = lineText.charCodeAt(offsetInLine);
						punctuation = _isPunctuation(c);
						if (unit === "wordend") {
							if (!punctuation && previousPunctuation) { break; }
						} else {
							if (punctuation && !previousPunctuation) { break; }
						}
						letterOrDigit  = !punctuation && !_isWhitespace(c);
						if (unit === "wordend") {
							if (!letterOrDigit && previousLetterOrDigit) { break; }
						} else {
							if (letterOrDigit && !previousLetterOrDigit) { break; }
						}
						previousLetterOrDigit = letterOrDigit;
						previousPunctuation = punctuation;
						offsetInLine++;
					}
				} else {
					if (offsetInLine === 0) { return lineStart; }
					offsetInLine--;
					c = lineText.charCodeAt(offsetInLine);
					previousPunctuation = _isPunctuation(c); 
					previousLetterOrDigit = !previousPunctuation && !_isWhitespace(c);
					while (0 < offsetInLine) {
						c = lineText.charCodeAt(offsetInLine - 1);
						punctuation = _isPunctuation(c);
						if (unit === "wordend") {
							if (punctuation && !previousPunctuation) { break; }
						} else {
							if (!punctuation && previousPunctuation) { break; }
						}
						letterOrDigit  = !punctuation && !_isWhitespace(c);
						if (unit === "wordend") {
							if (letterOrDigit && !previousLetterOrDigit) { break; }
						} else {
							if (!letterOrDigit && previousLetterOrDigit) { break; }
						}
						previousLetterOrDigit = letterOrDigit;
						previousPunctuation = punctuation;
						offsetInLine--;
					}
				}
				return lineStart + offsetInLine;
			}
			return offset + direction;
		},
		_getOffset_IE: function (offset, unit, direction) {
			var document = this._frameDocument;
			var model = this._model;
			var lineIndex = model.getLineAtOffset(offset);
			var clientDiv = this._clientDiv;
			var dummy;
			var child = this._getLineNode(lineIndex);
			if (!child) {
				child = dummy = this._createLine(clientDiv, null, document, lineIndex, model);
			}
			var result = 0, range, length;
			var lineOffset = model.getLineStart(lineIndex);
			if (offset === model.getLineEnd(lineIndex)) {
				range = document.body.createTextRange();
				range.moveToElementText(child.lastChild);
				length = range.text.length;
				range.moveEnd(unit, direction);
				result = offset + range.text.length - length;
			} else if (offset === lineOffset && direction < 0) {
				result = lineOffset;
			} else {
				var lineChild = child.firstChild;
				while (lineChild) {
					var textNode = lineChild.firstChild;
					var nodeLength = textNode.length;
					if (lineChild.ignoreChars) {
						nodeLength -= lineChild.ignoreChars;
					}
					if (lineOffset + nodeLength > offset) {
						range = document.body.createTextRange();
						if (offset === lineOffset && direction < 0) {
							range.moveToElementText(lineChild.previousSibling);
						} else {
							range.moveToElementText(lineChild);
							range.collapse();
							range.moveEnd("character", offset - lineOffset);
						}
						length = range.text.length;
						range.moveEnd(unit, direction);
						result = offset + range.text.length - length;
						break;
					}
					lineOffset = nodeLength + lineOffset;
					lineChild = lineChild.nextSibling;
				}
			}
			if (dummy) { clientDiv.removeChild(dummy); }
			return result;
		},
		_getOffsetToX: function (offset) {
			return this._getBoundsAtOffset(offset).left;
		},
		_getPadding: function (node) {
			var left,top,right,bottom;
			if (node.currentStyle) {
				left = node.currentStyle.paddingLeft;
				top = node.currentStyle.paddingTop;
				right = node.currentStyle.paddingRight;
				bottom = node.currentStyle.paddingBottom;
			} else if (this._frameWindow.getComputedStyle) {
				var style = this._frameWindow.getComputedStyle(node, null);
				left = style.getPropertyValue("padding-left");
				top = style.getPropertyValue("padding-top");
				right = style.getPropertyValue("padding-right");
				bottom = style.getPropertyValue("padding-bottom");
			}
			return {
					left: parseInt(left, 10), 
					top: parseInt(top, 10),
					right: parseInt(right, 10),
					bottom: parseInt(bottom, 10)
			};
		},
		_getScroll: function() {
			var viewDiv = this._viewDiv;
			return {x: viewDiv.scrollLeft, y: viewDiv.scrollTop};
		},
		_getSelection: function () {
			return this._selection.clone();
		},
		_getTopIndex: function (fullyVisible) {
			var child = this._topChild;
			if (fullyVisible && this._getClientHeight() > this._getLineHeight()) {
				var rect = child.getBoundingClientRect();
				var viewPad = this._getViewPadding();
				var viewRect = this._viewDiv.getBoundingClientRect();
				if (rect.top < viewRect.top + viewPad.top) {
					child = this._getLineNext(child) || child;
				}
			}
			return child.lineIndex;
		},
		_getXToOffset: function (lineIndex, x) {
			var model = this._model;
			var lineStart = model.getLineStart(lineIndex);
			var lineEnd = model.getLineEnd(lineIndex);
			if (lineStart === lineEnd) {
				return lineStart;
			}
			var document = this._frameDocument;
			var clientDiv = this._clientDiv;
			var dummy;
			var child = this._getLineNode(lineIndex);
			if (!child) {
				child = dummy = this._createLine(clientDiv, null, document, lineIndex, model);
			}
			var lineRect = this._getLineBoundingClientRect(child);
			if (x < lineRect.left) { x = lineRect.left; }
			if (x > lineRect.right) { x = lineRect.right; }
			



			var deltaX = 0, rects;
			if (isIE < 9) {
				rects = child.getClientRects();
				var minLeft = rects[0].left;
				for (var i=1; i<rects.length; i++) {
					minLeft = Math.min(rects[i].left, minLeft);
				}
				deltaX = minLeft - lineRect.left;
			}
			var scrollX = this._getScroll().x;
			function _getClientRects(element) {
				var rects, newRects, i, r;
				if (!element._rectsCache) {
					rects = element.getClientRects();
					newRects = [rects.length];
					for (i = 0; i<rects.length; i++) {
						r = rects[i];
						newRects[i] = {left: r.left - deltaX + scrollX, top: r.top, right: r.right - deltaX + scrollX, bottom: r.bottom};
					}
					element._rectsCache = newRects; 
				}
				rects = element._rectsCache;
				newRects = [rects.length];
				for (i = 0; i<rects.length; i++) {
					r = rects[i];
					newRects[i] = {left: r.left - scrollX, top: r.top, right: r.right - scrollX, bottom: r.bottom};
				}
				return newRects;
			}
			var logicalXDPI = isIE ? window.screen.logicalXDPI : 1;
			var deviceXDPI = isIE ? window.screen.deviceXDPI : 1;
			var offset = lineStart;
			var lineChild = child.firstChild;
			done:
			while (lineChild) {
				var textNode = lineChild.firstChild;
				var nodeLength = textNode.length;
				if (lineChild.ignoreChars) {
					nodeLength -= lineChild.ignoreChars;
				}
				rects = _getClientRects(lineChild);
				for (var j = 0; j < rects.length; j++) {
					var rect = rects[j];
					if (rect.left <= x && x < rect.right) {
						var range, start, end;
						if (isIE || isRangeRects) {
							range = isRangeRects ? document.createRange() : document.body.createTextRange();
							var high = nodeLength;
							var low = -1;
							while ((high - low) > 1) {
								var mid = Math.floor((high + low) / 2);
								start = low + 1;
								end = mid === nodeLength - 1 && lineChild.ignoreChars ? textNode.length : mid + 1;
								if (isRangeRects) {
									range.setStart(textNode, start);
									range.setEnd(textNode, end);
								} else {
									range.moveToElementText(lineChild);
									range.move("character", start);
									range.moveEnd("character", end - start);
								}
								rects = range.getClientRects();
								var found = false;
								for (var k = 0; k < rects.length; k++) {
									rect = rects[k];
									var rangeLeft = rect.left * logicalXDPI / deviceXDPI - deltaX;
									var rangeRight = rect.right * logicalXDPI / deviceXDPI - deltaX;
									if (rangeLeft <= x && x < rangeRight) {
										found = true;
										break;
									}
								}
								if (found) {
									high = mid;
								} else {
									low = mid;
								}
							}
							offset += high;
							start = high;
							end = high === nodeLength - 1 && lineChild.ignoreChars ? textNode.length : high + 1;
							if (isRangeRects) {
								range.setStart(textNode, start);
								range.setEnd(textNode, end);
							} else {
								range.moveToElementText(lineChild);
								range.move("character", start);
								range.moveEnd("character", end - start);
							}
							rect = range.getClientRects()[0];
							
							if (x > ((rect.left * logicalXDPI / deviceXDPI - deltaX) + ((rect.right - rect.left) * logicalXDPI / deviceXDPI / 2))) {
								offset++;
							}
						} else {
							var newText = [];
							for (var q = 0; q < nodeLength; q++) {
								newText.push("<span>");
								if (q === nodeLength - 1) {
									newText.push(textNode.data.substring(q));
								} else {
									newText.push(textNode.data.substring(q, q + 1));
								}
								newText.push("</span>");
							}
							lineChild.innerHTML = newText.join("");
							var rangeChild = lineChild.firstChild;
							while (rangeChild) {
								rect = rangeChild.getBoundingClientRect();
								if (rect.left <= x && x < rect.right) {
									
									if (x > rect.left + (rect.right - rect.left) / 2) {
										offset++;
									}
									break;
								}
								offset++;
								rangeChild = rangeChild.nextSibling;
							}
							if (!dummy) {
								lineChild.innerHTML = "";
								lineChild.appendChild(textNode);
								




								var s = this._getSelection();
								if ((offset <= s.start && s.start < offset + nodeLength) || (offset <= s.end && s.end < offset + nodeLength)) {
									this._updateDOMSelection();
								}
							}
						}
						break done;
					}
				}
				offset += nodeLength;
				lineChild = lineChild.nextSibling;
			}
			if (dummy) { clientDiv.removeChild(dummy); }
			return Math.min(lineEnd, Math.max(lineStart, offset));
		},
		_getYToLine: function (y) {
			var viewPad = this._getViewPadding();
			var viewRect = this._viewDiv.getBoundingClientRect();
			y -= viewRect.top + viewPad.top;
			var lineHeight = this._getLineHeight();
			var lineIndex = Math.floor((y + this._getScroll().y) / lineHeight);
			var lineCount = this._model.getLineCount();
			return Math.max(0, Math.min(lineCount - 1, lineIndex));
		},
		_getOffsetBounds: function(offset) {
			var model = this._model;
			var lineIndex = model.getLineAtOffset(offset);
			var lineHeight = this._getLineHeight();
			var scroll = this._getScroll();
			var viewPad = this._getViewPadding();
			var viewRect = this._viewDiv.getBoundingClientRect();
			var bounds = this._getBoundsAtOffset(offset);
			var left = bounds.left;
			var right = bounds.right;
			var top = (lineIndex * lineHeight) - scroll.y + viewRect.top + viewPad.top;
			var bottom = top + lineHeight;
			return {left: left, top: top, right: right, bottom: bottom};
		},
		_hitOffset: function (offset, x, y) {
			var bounds = this._getOffsetBounds(offset);
			var left = bounds.left;
			var right = bounds.right;
			var top = bounds.top;
			var bottom = bounds.bottom;
			var area = 20;
			left -= area;
			top -= area;
			right += area;
			bottom += area;
			return (left <= x && x <= right && top <= y && y <= bottom);
		},
		_hookEvents: function() {
			var self = this;
			this._modelListener = {
				
				onChanging: function(newText, start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
					self._onModelChanging(newText, start, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
				},
				
				onChanged: function(start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
					self._onModelChanged(start, removedCharCount, addedCharCount, removedLineCount, addedLineCount);
				}
			};
			this._model.addListener(this._modelListener);
			
			this._mouseMoveClosure = function(e) { return self._handleMouseMove(e);};
			this._mouseUpClosure = function(e) { return self._handleMouseUp(e);};
			
			var clientDiv = this._clientDiv;
			var viewDiv = this._viewDiv;
			var body = this._frameDocument.body; 
			var handlers = this._handlers = [];
			var resizeNode = isIE < 9 ? this._frame : this._frameWindow;
			var focusNode = isPad ? this._textArea : (isIE ||  isFirefox ? this._clientDiv: this._frameWindow);
			handlers.push({target: resizeNode, type: "resize", handler: function(e) { return self._handleResize(e);}});
			handlers.push({target: focusNode, type: "blur", handler: function(e) { return self._handleBlur(e);}});
			handlers.push({target: focusNode, type: "focus", handler: function(e) { return self._handleFocus(e);}});
			handlers.push({target: viewDiv, type: "scroll", handler: function(e) { return self._handleScroll(e);}});
			if (isPad) {
				var touchDiv = this._touchDiv;
				var textArea = this._textArea;
				handlers.push({target: textArea, type: "keydown", handler: function(e) { return self._handleKeyDown(e);}});
				handlers.push({target: textArea, type: "input", handler: function(e) { return self._handleInput(e); }});
				handlers.push({target: textArea, type: "textInput", handler: function(e) { return self._handleTextInput(e); }});
				handlers.push({target: touchDiv, type: "touchstart", handler: function(e) { return self._handleTouchStart(e); }});
				handlers.push({target: touchDiv, type: "touchmove", handler: function(e) { return self._handleTouchMove(e); }});
				handlers.push({target: touchDiv, type: "touchend", handler: function(e) { return self._handleTouchEnd(e); }});
			} else {
				var topNode = this._overlayDiv || this._clientDiv;
				handlers.push({target: clientDiv, type: "keydown", handler: function(e) { return self._handleKeyDown(e);}});
				handlers.push({target: clientDiv, type: "keypress", handler: function(e) { return self._handleKeyPress(e);}});
				handlers.push({target: clientDiv, type: "keyup", handler: function(e) { return self._handleKeyUp(e);}});
				handlers.push({target: clientDiv, type: "selectstart", handler: function(e) { return self._handleSelectStart(e);}});
				handlers.push({target: clientDiv, type: "contextmenu", handler: function(e) { return self._handleContextMenu(e);}});
				handlers.push({target: clientDiv, type: "copy", handler: function(e) { return self._handleCopy(e);}});
				handlers.push({target: clientDiv, type: "cut", handler: function(e) { return self._handleCut(e);}});
				handlers.push({target: clientDiv, type: "paste", handler: function(e) { return self._handlePaste(e);}});
				handlers.push({target: topNode, type: "mousedown", handler: function(e) { return self._handleMouseDown(e);}});
				handlers.push({target: body, type: "mousedown", handler: function(e) { return self._handleBodyMouseDown(e);}});
				handlers.push({target: topNode, type: "dragstart", handler: function(e) { return self._handleDragStart(e);}});
				handlers.push({target: topNode, type: "dragover", handler: function(e) { return self._handleDragOver(e);}});
				handlers.push({target: topNode, type: "drop", handler: function(e) { return self._handleDrop(e);}});
				if (isIE) {
					handlers.push({target: this._frameDocument, type: "activate", handler: function(e) { return self._handleDocFocus(e); }});
				}
				if (isFirefox) {
					handlers.push({target: this._frameDocument, type: "focus", handler: function(e) { return self._handleDocFocus(e); }});
				}
				if (!isIE && !isOpera) {
					var wheelEvent = isFirefox ? "DOMMouseScroll" : "mousewheel";
					handlers.push({target: this._viewDiv, type: wheelEvent, handler: function(e) { return self._handleMouseWheel(e); }});
				}
				if (isFirefox && !isWindows) {
					handlers.push({target: this._clientDiv, type: "DOMCharacterDataModified", handler: function (e) { return self._handleDataModified(e); }});
				}
				if (this._overlayDiv) {
					handlers.push({target: this._overlayDiv, type: "contextmenu", handler: function(e) { return self._handleContextMenu(e); }});
				}
				if (!isW3CEvents) {
					handlers.push({target: this._clientDiv, type: "dblclick", handler: function(e) { return self._handleDblclick(e); }});
				}
			}
			for (var i=0; i<handlers.length; i++) {
				var h = handlers[i];
				addHandler(h.target, h.type, h.handler, h.capture);
			}
		},
		_init: function(options) {
			var parent = options.parent;
			if (typeof(parent) === "string") {
				parent = window.document.getElementById(parent);
			}
			if (!parent) { throw "no parent"; }
			this._parent = parent;
			this._model = options.model ? options.model : new orion.textview.TextModel();
			this.readonly = options.readonly === true;
			this._selection = new Selection (0, 0, false);
			this._eventTable = new EventTable();
			this._maxLineWidth = 0;
			this._maxLineIndex = -1;
			this._ignoreSelect = true;
			this._columnX = -1;

			
			this._autoScrollX = null;
			this._autoScrollY = null;
			this._autoScrollTimerID = null;
			this._AUTO_SCROLL_RATE = 50;
			this._grabControl = null;
			this._moseMoveClosure  = null;
			this._mouseUpClosure = null;
			
			
			this._lastMouseX = 0;
			this._lastMouseY = 0;
			this._lastMouseTime = 0;
			this._clickCount = 0;
			this._clickTime = 250;
			this._clickDist = 5;
			this._isMouseDown = false;
			this._doubleClickSelection = null;
			
			
			this._hScroll = 0;
			this._vScroll = 0;

			
			this._imeOffset = -1;
			
			
			while (parent.hasChildNodes()) { parent.removeChild(parent.lastChild); }
			var parentDocument = parent.document || parent.ownerDocument;
			this._parentDocument = parentDocument;
			var frame = parentDocument.createElement("IFRAME");
			this._frame = frame;
			frame.frameBorder = "0px";
			frame.style.width = "100%";
			frame.style.height = "100%";
			frame.scrolling = "no";
			frame.style.border = "0px";
			parent.appendChild(frame);

			var html = [];
			html.push("<!DOCTYPE html>");
			html.push("<html>");
			html.push("<head>");
			if (isIE < 9) {
				html.push("<meta http-equiv='X-UA-Compatible' content='IE=EmulateIE7'/>");
			}
			html.push("<style>");
			html.push(".viewContainer {font-family: monospace; font-size: 10pt;}");
			html.push(".view {padding: 1px 2px;}");
			html.push(".viewContent {}");
			html.push("</style>");
			if (options.stylesheet) {
				var stylesheet = typeof(options.stylesheet) === "string" ? [options.stylesheet] : options.stylesheet;
				for (var i = 0; i < stylesheet.length; i++) {
					try {
						
						var objXml = new XMLHttpRequest();
						if (objXml.overrideMimeType) {
							objXml.overrideMimeType("text/css");
						}
						objXml.open("GET", stylesheet[i], false);
						objXml.send(null);
						html.push("<style>");
						html.push(objXml.responseText);
						html.push("</style>");
					} catch (e) {
						html.push("<link rel='stylesheet' type='text/css' href='");
						html.push(stylesheet[i]);
						html.push("'></link>");
					}
				}
			}
			html.push("</head>");
			html.push("<body spellcheck='false'></body>");
			html.push("</html>");

			var frameWindow = frame.contentWindow;
			this._frameWindow = frameWindow;
			var document = frameWindow.document;
			this._frameDocument = document;
			document.open();
			document.write(html.join(""));
			document.close();
			
			var body = document.body;
			body.className = "viewContainer";
			body.style.margin = "0px";
			body.style.borderWidth = "0px";
			body.style.padding = "0px";
			
			if (isPad) {
				var touchDiv = parentDocument.createElement("DIV");
				this._touchDiv = touchDiv;
				touchDiv.style.position = "absolute";
				touchDiv.style.border = "0px";
				touchDiv.style.padding = "0px";
				touchDiv.style.margin = "0px";
				touchDiv.style.zIndex = "2";
				touchDiv.style.overflow = "hidden";
				touchDiv.style.background="transparent";
				touchDiv.style.WebkitUserSelect = "none";
				parent.appendChild(touchDiv);

				var textArea = parentDocument.createElement("TEXTAREA");
				this._textArea = textArea;
				textArea.style.position = "absolute";
				textArea.style.whiteSpace = "pre";
				textArea.style.left = "-1000px";
				textArea.tabIndex = 1;
				textArea.autocapitalize = false;
				textArea.autocorrect = false;
				textArea.className = "viewContainer";
				textArea.style.background = "transparent";
				textArea.style.color = "transparent";
				textArea.style.border = "0px";
				textArea.style.padding = "0px";
				textArea.style.margin = "0px";
				textArea.style.borderRadius = "0px";
				textArea.style.WebkitAppearance = "none";
				textArea.style.WebkitTapHighlightColor = "transparent";
				touchDiv.appendChild(textArea);
			}

			var viewDiv = document.createElement("DIV");
			viewDiv.className = "view";
			this._viewDiv = viewDiv;
			viewDiv.id = "viewDiv";
			viewDiv.tabIndex = -1;
			viewDiv.style.overflow = "auto";
			viewDiv.style.position = "absolute";
			viewDiv.style.top = "0px";
			viewDiv.style.borderWidth = "0px";
			viewDiv.style.margin = "0px";
			viewDiv.style.MozOutline = "none";
			viewDiv.style.outline = "none";
			body.appendChild(viewDiv);
				
			var scrollDiv = document.createElement("DIV");
			this._scrollDiv = scrollDiv;
			scrollDiv.id = "scrollDiv";
			scrollDiv.style.margin = "0px";
			scrollDiv.style.borderWidth = "0px";
			scrollDiv.style.padding = "0px";
			viewDiv.appendChild(scrollDiv);

			this._fullSelection = options.fullSelection === undefined || options.fullSelection;
			




			if (isIE < 9) {
				this._fullSelection = false;
			}
			if (isPad || (this._fullSelection && !isWebkit)) {
				this._hightlightRGB = "Highlight";
				var selDiv1 = document.createElement("DIV");
				this._selDiv1 = selDiv1;
				selDiv1.id = "selDiv1";
				selDiv1.style.position = "fixed";
				selDiv1.style.borderWidth = "0px";
				selDiv1.style.margin = "0px";
				selDiv1.style.padding = "0px";
				selDiv1.style.MozOutline = "none";
				selDiv1.style.outline = "none";
				selDiv1.style.background = this._hightlightRGB;
				selDiv1.style.width="0px";
				selDiv1.style.height="0px";
				scrollDiv.appendChild(selDiv1);
				var selDiv2 = document.createElement("DIV");
				this._selDiv2 = selDiv2;
				selDiv2.id = "selDiv2";
				selDiv2.style.position = "fixed";
				selDiv2.style.borderWidth = "0px";
				selDiv2.style.margin = "0px";
				selDiv2.style.padding = "0px";
				selDiv2.style.MozOutline = "none";
				selDiv2.style.outline = "none";
				selDiv2.style.background = this._hightlightRGB;
				selDiv2.style.width="0px";
				selDiv2.style.height="0px";
				scrollDiv.appendChild(selDiv2);
				var selDiv3 = document.createElement("DIV");
				this._selDiv3 = selDiv3;
				selDiv3.id = "selDiv3";
				selDiv3.style.position = "fixed";
				selDiv3.style.borderWidth = "0px";
				selDiv3.style.margin = "0px";
				selDiv3.style.padding = "0px";
				selDiv3.style.MozOutline = "none";
				selDiv3.style.outline = "none";
				selDiv3.style.background = this._hightlightRGB;
				selDiv3.style.width="0px";
				selDiv3.style.height="0px";
				scrollDiv.appendChild(selDiv3);
				
				




				if (isFirefox && isMac) {
					var style = frameWindow.getComputedStyle(selDiv3, null);
					var rgb = style.getPropertyValue("background-color");
					switch (rgb) {
						case "rgb(119, 141, 168)": rgb = "rgb(199, 208, 218)"; break;
						case "rgb(127, 127, 127)": rgb = "rgb(198, 198, 198)"; break;
						case "rgb(255, 193, 31)": rgb = "rgb(250, 236, 115)"; break;
						case "rgb(243, 70, 72)": rgb = "rgb(255, 176, 139)"; break;
						case "rgb(255, 138, 34)": rgb = "rgb(255, 209, 129)"; break;
						case "rgb(102, 197, 71)": rgb = "rgb(194, 249, 144)"; break;
						case "rgb(140, 78, 184)": rgb = "rgb(232, 184, 255)"; break;
						default: rgb = "rgb(180, 213, 255)"; break;
					}
					this._hightlightRGB = rgb;
					selDiv1.style.background = rgb;
					selDiv2.style.background = rgb;
					selDiv3.style.background = rgb;
					var styleSheet = document.styleSheets[0];
					styleSheet.insertRule("::-moz-selection {background: " + rgb + "; }", 0);
				}
			}

			var clientDiv = document.createElement("DIV");
			clientDiv.className = "viewContent";
			this._clientDiv = clientDiv;
			clientDiv.id = "clientDiv";
			clientDiv.style.whiteSpace = "pre";
			clientDiv.style.position = "fixed";
			clientDiv.style.borderWidth = "0px";
			clientDiv.style.margin = "0px";
			clientDiv.style.padding = "0px";
			clientDiv.style.MozOutline = "none";
			clientDiv.style.outline = "none";
			if (isPad) {
				clientDiv.style.WebkitTapHighlightColor = "transparent";
			}
			scrollDiv.appendChild(clientDiv);

			if (isFirefox) {
				var overlayDiv = document.createElement("DIV");
				this._overlayDiv = overlayDiv;
				overlayDiv.id = "overlayDiv";
				overlayDiv.style.position = clientDiv.style.position;
				overlayDiv.style.borderWidth = clientDiv.style.borderWidth;
				overlayDiv.style.margin = clientDiv.style.margin;
				overlayDiv.style.padding = clientDiv.style.padding;
				overlayDiv.style.cursor = "text";
				overlayDiv.style.zIndex = "1";
				scrollDiv.appendChild(overlayDiv);
			}
			if (!isPad) {
				clientDiv.contentEditable = "true";
			}
			this._lineHeight = this._calculateLineHeight();
			this._viewPadding = this._calculatePadding();
			if (isIE) {
				body.style.lineHeight = this._lineHeight + "px";
			}
			if (options.tabSize) {
				if (isOpera) {
					clientDiv.style.OTabSize = options.tabSize+"";
				} else if (isFirefox >= 4) {
					clientDiv.style.MozTabSize = options.tabSize+"";
				} else if (options.tabSize !== 8) {
					this._tabSize = options.tabSize;
				}
			}
			this._createActions();
			this._hookEvents();
			this._updatePage();
		},
		_modifyContent: function(e, updateCaret) {
			if (this.readonly && !e._code) {
				return;
			}

			this.onVerify(e);

			if (e.text === null || e.text === undefined) { return; }
			
			var model = this._model;
			if (e._ignoreDOMSelection) { this._ignoreDOMSelection = true; }
			model.setText (e.text, e.start, e.end);
			if (e._ignoreDOMSelection) { this._ignoreDOMSelection = false; }
			
			if (updateCaret) {
				var selection = this._getSelection ();
				selection.setCaret(e.start + e.text.length);
				this._setSelection(selection, true);
			}
			this.onModify({});
		},
		_onModelChanged: function(start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
			var e = {
				start: start,
				removedCharCount: removedCharCount,
				addedCharCount: addedCharCount,
				removedLineCount: removedLineCount,
				addedLineCount: addedLineCount
			};
			this.onModelChanged(e);
			
			var selection = this._getSelection();
			if (selection.end > start) {
				if (selection.end > start && selection.start < start + removedCharCount) {
					
					selection.setCaret(start + addedCharCount);
				} else {
					
					selection.start +=  addedCharCount - removedCharCount;
					selection.end +=  addedCharCount - removedCharCount;
				}
				this._setSelection(selection, false, false);
			}
			
			var model = this._model;
			var startLine = model.getLineAtOffset(start);
			var child = this._getLineNext();
			while (child) {
				var lineIndex = child.lineIndex;
				if (startLine <= lineIndex && lineIndex <= startLine + removedLineCount) {
					child.lineChanged = true;
				}
				if (lineIndex > startLine + removedLineCount) {
					child.lineIndex = lineIndex + addedLineCount - removedLineCount;
				}
				child = this._getLineNext(child);
			}
			if (startLine <= this._maxLineIndex && this._maxLineIndex <= startLine + removedLineCount) {
				this._maxLineIndex = -1;
				this._maxLineWidth = 0;
			}
			this._updatePage();
		},
		_onModelChanging: function(newText, start, removedCharCount, addedCharCount, removedLineCount, addedLineCount) {
			var e = {
				text: newText,
				start: start,
				removedCharCount: removedCharCount,
				addedCharCount: addedCharCount,
				removedLineCount: removedLineCount,
				addedLineCount: addedLineCount
			};
			this.onModelChanging(e);
		},
		_queueUpdatePage: function() {
			if (this._updateTimer) { return; }
			var self = this;
			this._updateTimer = setTimeout(function() { 
				self._updateTimer = null;
				self._updatePage();
			}, 0);
		},
		_resizeTouchDiv: function() {
			var viewRect = this._viewDiv.getBoundingClientRect();
			var parentRect = this._frame.getBoundingClientRect();
			var temp = this._frame;
			while (temp) {
				if (temp.style && temp.style.top) { break; }
				temp = temp.parentNode;
			}
			var parentTop = parentRect.top;
			if (temp) {
				parentTop -= temp.getBoundingClientRect().top;
			} else {
				parentTop += this._parentDocument.body.scrollTop;
			}
			temp = this._frame;
			while (temp) {
				if (temp.style && temp.style.left) { break; }
				temp = temp.parentNode;
			}
			var parentLeft = parentRect.left;
			if (temp) {
				parentLeft -= temp.getBoundingClientRect().left;
			} else {
				parentLeft += this._parentDocument.body.scrollLeft;
			}
			var touchDiv = this._touchDiv;
			touchDiv.style.left = (parentLeft + viewRect.left) + "px";
			touchDiv.style.top = (parentTop + viewRect.top) + "px";
			touchDiv.style.width = viewRect.width + "px";
			touchDiv.style.height = viewRect.height + "px";
		},
		_scrollView: function (pixelX, pixelY) {
			



			this._ensureCaretVisible = false;
			
			








			var viewDiv = this._viewDiv;
			if (pixelX) { viewDiv.scrollLeft += pixelX; }
			if (pixelY) { viewDiv.scrollTop += pixelY; }
		},
		_setClipboardText: function (text, event) {
			var clipboardText;
			if (this._frameWindow.clipboardData) {
				
				clipboardText = [];
				this._convertDelimiter(text, function(t) {clipboardText.push(t);}, function() {clipboardText.push(platformDelimiter);});
				return this._frameWindow.clipboardData.setData("Text", clipboardText.join(""));
			}
			
			if (isChrome || isFirefox || !event) {
				var window = this._frameWindow;
				var document = this._frameDocument;
				var child = document.createElement("PRE");
				child.style.position = "fixed";
				child.style.left = "-1000px";
				this._convertDelimiter(text, 
					function(t) {
						child.appendChild(document.createTextNode(t));
					}, 
					function() {
						child.appendChild(document.createElement("BR"));
					}
				);
				child.appendChild(document.createTextNode(" "));
				this._clientDiv.appendChild(child);
				var range = document.createRange();
				range.setStart(child.firstChild, 0);
				range.setEndBefore(child.lastChild);
				var sel = window.getSelection();
				if (sel.rangeCount > 0) { sel.removeAllRanges(); }
				sel.addRange(range);
				var self = this;
				
				var cleanup = function() {
					self._clientDiv.removeChild(child);
					self._updateDOMSelection();
				};
				var result = false;
				



				this._ignoreCopy = true;
				try {
					result = document.execCommand("copy", false, null);
				} catch (e) {}
				this._ignoreCopy = false;
				if (!result) {
					if (event) {
						setTimeout(cleanup, 0);
						return false;
					}
				}
				
				cleanup();
				return true;
			}
			if (event && event.clipboardData) {
				
				clipboardText = [];
				this._convertDelimiter(text, function(t) {clipboardText.push(t);}, function() {clipboardText.push(platformDelimiter);});
				return event.clipboardData.setData("text/plain", clipboardText.join("")); 
			}
		},
		_setDOMSelection: function (startNode, startOffset, endNode, endOffset) {
			var window = this._frameWindow;
			var document = this._frameDocument;
			var startLineNode, startLineOffset, endLineNode, endLineOffset;
			var offset = 0;
			var lineChild = startNode.firstChild;
			var node, nodeLength, model = this._model;
			var startLineEnd = model.getLine(startNode.lineIndex).length;
			while (lineChild) {
				node = lineChild.firstChild;
				nodeLength = node.length;
				if (lineChild.ignoreChars) {
					nodeLength -= lineChild.ignoreChars;
				}
				if (offset + nodeLength > startOffset || offset + nodeLength >= startLineEnd) {
					startLineNode = node;
					startLineOffset = startOffset - offset;
					if (lineChild.ignoreChars && nodeLength > 0 && startLineOffset === nodeLength) {
						startLineOffset += lineChild.ignoreChars; 
					}
					break;
				}
				offset += nodeLength;
				lineChild = lineChild.nextSibling;
			}
			offset = 0;
			lineChild = endNode.firstChild;
			var endLineEnd = this._model.getLine(endNode.lineIndex).length;
			while (lineChild) {
				node = lineChild.firstChild;
				nodeLength = node.length;
				if (lineChild.ignoreChars) {
					nodeLength -= lineChild.ignoreChars;
				}
				if (nodeLength + offset > endOffset || offset + nodeLength >= endLineEnd) {
					endLineNode = node;
					endLineOffset = endOffset - offset;
					if (lineChild.ignoreChars && nodeLength > 0 && endLineOffset === nodeLength) {
						endLineOffset += lineChild.ignoreChars; 
					}
					break;
				}
				offset += nodeLength;
				lineChild = lineChild.nextSibling;
			}
			
			this._setDOMFullSelection(startNode, startOffset, startLineEnd, endNode, endOffset, endLineEnd);
			if (isPad) { return; }

			var range;
			if (window.getSelection) {
				
				range = document.createRange();
				range.setStart(startLineNode, startLineOffset);
				range.setEnd(endLineNode, endLineOffset);
				var sel = window.getSelection();
				this._ignoreSelect = false;
				if (sel.rangeCount > 0) { sel.removeAllRanges(); }
				sel.addRange(range);
				this._ignoreSelect = true;
			} else if (document.selection) {
				
				var body = document.body;

				




				var child = document.createElement("DIV");
				body.appendChild(child);
				body.removeChild(child);
				
				range = body.createTextRange();
				range.moveToElementText(startLineNode.parentNode);
				range.moveStart("character", startLineOffset);
				var endRange = body.createTextRange();
				endRange.moveToElementText(endLineNode.parentNode);
				endRange.moveStart("character", endLineOffset);
				range.setEndPoint("EndToStart", endRange);
				this._ignoreSelect = false;
				range.select();
				this._ignoreSelect = true;
			}
		},
		_setDOMFullSelection: function(startNode, startOffset, startLineEnd, endNode, endOffset, endLineEnd) {
			var model = this._model;
			if (this._selDiv1) {
				var startLineBounds, l;
				startLineBounds = this._getLineBoundingClientRect(startNode);
				if (startOffset === 0) {
					l = startLineBounds.left;
				} else {
					if (startOffset >= startLineEnd) {
						l = startLineBounds.right;
					} else {
						this._ignoreDOMSelection = true;
						l = this._getBoundsAtOffset(model.getLineStart(startNode.lineIndex) + startOffset).left;
						this._ignoreDOMSelection = false;
					}
				}
				var textArea = this._textArea;
				if (textArea) {
					textArea.selectionStart = textArea.selectionEnd = 0;
					var rect = this._frame.getBoundingClientRect();
					var touchRect = this._touchDiv.getBoundingClientRect();
					var viewBounds = this._viewDiv.getBoundingClientRect();
					if (!(viewBounds.left <= l && l <= viewBounds.left + viewBounds.width &&
						viewBounds.top <= startLineBounds.top && startLineBounds.top <= viewBounds.top + viewBounds.height) ||
						!(startNode === endNode && startOffset === endOffset))
					{
						textArea.style.left = "-1000px";
					} else {
						textArea.style.left = (l - 4 + rect.left - touchRect.left) + "px";
					}
					textArea.style.top = (startLineBounds.top + rect.top - touchRect.top) + "px";
					textArea.style.width = "6px";
					textArea.style.height = (startLineBounds.bottom - startLineBounds.top) + "px";
				}
			
				var selDiv = this._selDiv1;
				selDiv.style.width = "0px";
				selDiv.style.height = "0px";
				selDiv = this._selDiv2;
				selDiv.style.width = "0px";
				selDiv.style.height = "0px";
				selDiv = this._selDiv3;
				selDiv.style.width = "0px";
				selDiv.style.height = "0px";
				if (!(startNode === endNode && startOffset === endOffset)) {
					var handleWidth = isPad ? 2 : 0;
					var handleBorder = handleWidth + "px blue solid";
					var viewPad = this._getViewPadding();
					var clientRect = this._clientDiv.getBoundingClientRect();
					var viewRect = this._viewDiv.getBoundingClientRect();
					var left = viewRect.left + viewPad.left;
					var right = clientRect.right;
					var top = viewRect.top + viewPad.top;
					var bottom = clientRect.bottom;
					var r;
					var endLineBounds = this._getLineBoundingClientRect(endNode);
					if (endOffset === 0) {
						r = endLineBounds.left;
					} else {
						if (endOffset >= endLineEnd) {
							r = endLineBounds.right;
						} else {
							this._ignoreDOMSelection = true;
							r = this._getBoundsAtOffset(model.getLineStart(endNode.lineIndex) + endOffset).left;
							this._ignoreDOMSelection = false;
						}
					}
					var sel1Div = this._selDiv1;
					var sel1Left = Math.min(right, Math.max(left, l));
					var sel1Top = Math.min(bottom, Math.max(top, startLineBounds.top));
					var sel1Right = right;
					var sel1Bottom = Math.min(bottom, Math.max(top, startLineBounds.bottom));
					sel1Div.style.left = sel1Left + "px";
					sel1Div.style.top = sel1Top + "px";
					sel1Div.style.width = Math.max(0, sel1Right - sel1Left) + "px";
					sel1Div.style.height = Math.max(0, sel1Bottom - sel1Top) + (isPad ? 1 : 0) + "px";
					if (isPad) {
						sel1Div.style.borderLeft = handleBorder;
						sel1Div.style.borderRight = "0px";
					}
					if (startNode === endNode) {
						sel1Right = Math.min(r, right);
						sel1Div.style.width = Math.max(0, sel1Right - sel1Left - handleWidth * 2) + "px";
						if (isPad) {
							sel1Div.style.borderRight = handleBorder;
						}
					} else {
						var sel3Left = left;
						var sel3Top = Math.min(bottom, Math.max(top, endLineBounds.top));
						var sel3Right = Math.min(right, Math.max(left, r));
						var sel3Bottom = Math.min(bottom, Math.max(top, endLineBounds.bottom));
						var sel3Div = this._selDiv3;
						sel3Div.style.left = sel3Left + "px";
						sel3Div.style.top = sel3Top + "px";
						sel3Div.style.width = Math.max(0, sel3Right - sel3Left - handleWidth) + "px";
						sel3Div.style.height = Math.max(0, sel3Bottom - sel3Top) + "px";
						if (isPad) {
							sel3Div.style.borderRight = handleBorder;
						}
						if (sel3Top - sel1Bottom > 0) {
							var sel2Div = this._selDiv2;
							sel2Div.style.left = left + "px";
							sel2Div.style.top = sel1Bottom + "px";
							sel2Div.style.width = Math.max(0, right - left) + "px";
							sel2Div.style.height = Math.max(0, sel3Top - sel1Bottom) + (isPad ? 1 : 0) + "px";
						}
					}
				}
			}
		},
		_setGrab: function (target) {
			if (target === this._grabControl) { return; }
			if (target) {
				addHandler(target, "mousemove", this._mouseMoveClosure);
				addHandler(target, "mouseup", this._mouseUpClosure);
				if (target.setCapture) { target.setCapture(); }
				this._grabControl = target;
			} else {
				removeHandler(this._grabControl, "mousemove", this._mouseMoveClosure);
				removeHandler(this._grabControl, "mouseup", this._mouseUpClosure);
				if (this._grabControl.releaseCapture) { this._grabControl.releaseCapture(); }
				this._grabControl = null;
			}
		},
		_setSelection: function (selection, scroll, update) {
			if (selection) {
				this._columnX = -1;
				if (update === undefined) { update = true; }
				var oldSelection = this._selection; 
				if (!oldSelection.equals(selection)) {
					this._selection = selection;
					var e = {
						oldValue: {start:oldSelection.start, end:oldSelection.end},
						newValue: {start:selection.start, end:selection.end}
					};
					this.onSelection(e);
				}
				





				if (scroll) { update = !this._showCaret(); }
				
				





				if (update) { this._updateDOMSelection(); }
			}
		},
		_setSelectionTo: function (x,y,extent) {
			var model = this._model, offset;
			var selection = this._getSelection();
			var lineIndex = this._getYToLine(y);
			if (this._clickCount === 1) {
				offset = this._getXToOffset(lineIndex, x);
				selection.extend(offset);
				if (!extent) { selection.collapse(); }
			} else {
				var word = (this._clickCount & 1) === 0;
				var start, end;
				if (word) {
					offset = this._getXToOffset(lineIndex, x);
					if (this._doubleClickSelection) {
						if (offset >= this._doubleClickSelection.start) {
							start = this._doubleClickSelection.start;
							end = this._getOffset(offset, "wordend", +1);
						} else {
							start = this._getOffset(offset, "word", -1);
							end = this._doubleClickSelection.end;
						}
					} else {
						start = this._getOffset(offset, "word", -1);
						end = this._getOffset(start, "wordend", +1);
					}
				} else {
					if (this._doubleClickSelection) {
						var doubleClickLine = model.getLineAtOffset(this._doubleClickSelection.start);
						if (lineIndex >= doubleClickLine) {
							start = model.getLineStart(doubleClickLine);
							end = model.getLineEnd(lineIndex);
						} else {
							start = model.getLineStart(lineIndex);
							end = model.getLineEnd(doubleClickLine);
						}
					} else {
						start = model.getLineStart(lineIndex);
						end = model.getLineEnd(lineIndex);
					}
				}
				selection.setCaret(start);
				selection.extend(end);
			} 
			this._setSelection(selection, true, true);
		},
		_showCaret: function () {
			var model = this._model;
			var selection = this._getSelection();
			var scroll = this._getScroll();
			var caret = selection.getCaret();
			var start = selection.start;
			var end = selection.end;
			var startLine = model.getLineAtOffset(start); 
			var endLine = model.getLineAtOffset(end);
			var endInclusive = Math.max(Math.max(start, model.getLineStart(endLine)), end - 1);
			var viewPad = this._getViewPadding();
			
			var clientWidth = this._getClientWidth();
			var leftEdge = viewPad.left;
			var rightEdge = viewPad.left + clientWidth;
			var bounds = this._getBoundsAtOffset(caret === start ? start : endInclusive);
			var left = bounds.left;
			var right = bounds.right;
			var minScroll = clientWidth / 4;
			if (!selection.isEmpty() && startLine === endLine) {
				bounds = this._getBoundsAtOffset(caret === end ? start : endInclusive);
				var selectionWidth = caret === start ? bounds.right - left : right - bounds.left;
				if ((clientWidth - minScroll) > selectionWidth) {
					if (left > bounds.left) { left = bounds.left; }
					if (right < bounds.right) { right = bounds.right; }
				}
			}
			var viewRect = this._viewDiv.getBoundingClientRect(); 
			left -= viewRect.left;
			right -= viewRect.left;
			var pixelX = 0;
			if (left < leftEdge) {
				pixelX = Math.min(left - leftEdge, -minScroll);
			}
			if (right > rightEdge) {
				var maxScroll = this._scrollDiv.scrollWidth - scroll.x - clientWidth;
				pixelX = Math.min(maxScroll,  Math.max(right - rightEdge, minScroll));
			}

			var pixelY = 0;
			var topIndex = this._getTopIndex(true);
			var bottomIndex = this._getBottomIndex(true);
			var caretLine = model.getLineAtOffset(caret);
			var clientHeight = this._getClientHeight();
			if (!(topIndex <= caretLine && caretLine <= bottomIndex)) {
				var lineHeight = this._getLineHeight();
				var selectionHeight = (endLine - startLine) * lineHeight;
				pixelY = caretLine * lineHeight;
				pixelY -= scroll.y;
				if (pixelY + lineHeight > clientHeight) {
					pixelY -= clientHeight - lineHeight;
					if (caret === start && start !== end) {
						pixelY += Math.min(clientHeight - lineHeight, selectionHeight);
					}
				} else {
					if (caret === end) {
						pixelY -= Math.min (clientHeight - lineHeight, selectionHeight);
					}
				}
			}

			if (pixelX !== 0 || pixelY !== 0) {
				this._scrollView (pixelX, pixelY);
				





				if (clientHeight !== this._getClientHeight() || clientWidth !== this._getClientWidth()) {
					this._showCaret();
				} else {
					this._ensureCaretVisible = true;
				}
				return true;
			}
			return false;
		},
		_startIME: function () {
			if (this._imeOffset !== -1) { return; }
			var selection = this._getSelection();
			if (!selection.isEmpty()) {
				this._modifyContent({text: "", start: selection.start, end: selection.end}, true);
			}
			this._imeOffset = selection.start;
		},
		_unhookEvents: function() {
			this._model.removeListener(this._modelListener);
			this._modelListener = null;

			this._mouseMoveClosure = null;
			this._mouseUpClosure = null;

			for (var i=0; i<this._handlers.length; i++) {
				var h = this._handlers[i];
				removeHandler(h.target, h.type, h.handler);
			}
			this._handlers = null;
		},
		_updateDOMSelection: function () {
			if (this._ignoreDOMSelection) { return; }
			var selection = this._getSelection();
			var model = this._model;
			var startLine = model.getLineAtOffset(selection.start);
			var endLine = model.getLineAtOffset(selection.end);
			var firstNode = this._getLineNext();
			



			if (!firstNode) { return; }
			var lastNode = this._getLinePrevious();
			
			var topNode, bottomNode, topOffset, bottomOffset;
			if (startLine < firstNode.lineIndex) {
				topNode = firstNode;
				topOffset = 0;
			} else if (startLine > lastNode.lineIndex) {
				topNode = lastNode;
				topOffset = 0;
			} else {
				topNode = this._getLineNode(startLine);
				topOffset = selection.start - model.getLineStart(startLine);
			}

			if (endLine < firstNode.lineIndex) {
				bottomNode = firstNode;
				bottomOffset = 0;
			} else if (endLine > lastNode.lineIndex) {
				bottomNode = lastNode;
				bottomOffset = 0;
			} else {
				bottomNode = this._getLineNode(endLine);
				bottomOffset = selection.end - model.getLineStart(endLine);
			}
			this._setDOMSelection(topNode, topOffset, bottomNode, bottomOffset);
		},
		_updatePage: function() {
			if (this._updateTimer) { 
				clearTimeout(this._updateTimer);
				this._updateTimer = null;
			}
			var document = this._frameDocument;
			var frameWidth = this._getFrameWidth();
			var frameHeight = this._getFrameHeight();
			document.body.style.width = frameWidth + "px";
			document.body.style.height = frameHeight + "px";
			
			var viewDiv = this._viewDiv;
			var clientDiv = this._clientDiv;
			var viewPad = this._getViewPadding();
			
			
			viewDiv.style.height = Math.max(0, (frameHeight - viewPad.top - viewPad.bottom)) + "px";
			
			var model = this._model;
			var lineHeight = this._getLineHeight();
			var scrollY = this._getScroll().y;
			var firstLine = Math.max(0, scrollY) / lineHeight;
			var topIndex = Math.floor(firstLine);
			var lineStart = Math.max(0, topIndex - 1);
			var top = Math.round((firstLine - lineStart) * lineHeight);
			var lineCount = model.getLineCount();
			var clientHeight = this._getClientHeight();
			var partialY = Math.round((firstLine - topIndex) * lineHeight);
			var linesPerPage = Math.floor((clientHeight + partialY) / lineHeight);
			var bottomIndex = Math.min(topIndex + linesPerPage, lineCount - 1);
			var lineEnd = Math.min(bottomIndex + 1, lineCount - 1);
			this._partialY = partialY;
			
			var lineIndex, lineWidth;
			var child = clientDiv.firstChild;
			while (child) {
				lineIndex = child.lineIndex;
				var nextChild = child.nextSibling;
				if (!(lineStart <= lineIndex && lineIndex <= lineEnd) || child.lineChanged || child.lineIndex === -1) {
					if (this._mouseWheelLine === child) {
						child.style.display = "none";
						child.lineIndex = -1;
					} else {
						clientDiv.removeChild(child);
					}
				}
				child = nextChild;
			}

			child = this._getLineNext();
			var frag = document.createDocumentFragment();
			for (lineIndex=lineStart; lineIndex<=lineEnd; lineIndex++) {
				if (!child || child.lineIndex > lineIndex) {
					this._createLine(frag, null, document, lineIndex, model);
				} else {
					if (frag.firstChild) {
						clientDiv.insertBefore(frag, child);
						frag = document.createDocumentFragment();
					}
					child = this._getLineNext(child);
				}
			}
			if (frag.firstChild) { clientDiv.insertBefore(frag, child); }

			






 
			if (isWebkit) {
				clientDiv.style.width = (0x7FFFF).toString() + "px";
			}

			child = this._getLineNext();
			while (child) {
				lineWidth = child.lineWidth;
				if (lineWidth === undefined) {
					var rect = this._getLineBoundingClientRect(child);
					lineWidth = child.lineWidth = rect.right - rect.left;
				}
				if (lineWidth >= this._maxLineWidth) {
					this._maxLineWidth = lineWidth;
					this._maxLineIndex = child.lineIndex;
				}
				if (child.lineIndex === topIndex) { this._topChild = child; }
				if (child.lineIndex === bottomIndex) { this._bottomChild = child; }
				child = this._getLineNext(child);
			}

			
			this._updateRuler(this._leftDiv, topIndex, bottomIndex);
			this._updateRuler(this._rightDiv, topIndex, bottomIndex);
			
			var leftWidth = this._leftDiv ? this._leftDiv.scrollWidth : 0;
			var rightWidth = this._rightDiv ? this._rightDiv.scrollWidth : 0;
			viewDiv.style.left = leftWidth + "px";
			viewDiv.style.width = Math.max(0, frameWidth - leftWidth - rightWidth - viewPad.left - viewPad.right) + "px";
			if (this._rightDiv) {
				this._rightDiv.style.left = (frameWidth - rightWidth) + "px"; 
			}
			
			var scrollDiv = this._scrollDiv;
			
			var scrollHeight = lineCount * lineHeight;
			scrollDiv.style.height = scrollHeight + "px";
			var clientWidth = this._getClientWidth();
			var width = Math.max(this._maxLineWidth, clientWidth);
			



			var scrollWidth = width;
			if (!isIE || isIE >= 9) { width += viewPad.right; }
			scrollDiv.style.width = width + "px";

			
			var scroll = this._getScroll();
			var left = scroll.x;
			var clipLeft = left;
			var clipTop = top;
			var clipRight = left + clientWidth;
			var clipBottom = top + clientHeight;
			if (clipLeft === 0) { clipLeft -= viewPad.left; }
			if (clipTop === 0) { clipTop -= viewPad.top; }
			if (clipRight === scrollWidth) { clipRight += viewPad.right; }
			if (scroll.y + clientHeight === scrollHeight) { clipBottom += viewPad.bottom; }
			clientDiv.style.clip = "rect(" + clipTop + "px," + clipRight + "px," + clipBottom + "px," + clipLeft + "px)";
			clientDiv.style.left = (-left + leftWidth + viewPad.left) + "px";
			clientDiv.style.top = (-top + viewPad.top) + "px";
			clientDiv.style.width = (isWebkit ? scrollWidth : clientWidth + left) + "px";
			clientDiv.style.height = (clientHeight + top) + "px";
			var overlayDiv = this._overlayDiv;
			if (overlayDiv) {
				overlayDiv.style.clip = clientDiv.style.clip;
				overlayDiv.style.left = clientDiv.style.left;
				overlayDiv.style.top = clientDiv.style.top;
				overlayDiv.style.width = clientDiv.style.width;
				overlayDiv.style.height = clientDiv.style.height;
			}
			function _updateRulerSize(divRuler) {
				if (!divRuler) { return; }
				var rulerHeight = clientHeight + viewPad.top + viewPad.bottom;
				var cells = divRuler.firstChild.rows[0].cells;
				for (var i = 0; i < cells.length; i++) {
					var div = cells[i].firstChild;
					var offset = lineHeight;
					if (div._ruler.getOverview() === "page") { offset += partialY; }
					div.style.top = -offset + "px";
					div.style.height = (rulerHeight + offset) + "px";
					div = div.nextSibling;
				}
				divRuler.style.height = rulerHeight + "px";
			}
			_updateRulerSize(this._leftDiv);
			_updateRulerSize(this._rightDiv);
			if (isPad) {
				var self = this;
				setTimeout(function() {self._resizeTouchDiv();}, 0);
			}
			this._updateDOMSelection();

			






			var ensureCaretVisible = this._ensureCaretVisible;
			this._ensureCaretVisible = false;
			if (clientHeight !== this._getClientHeight()) {
				this._updatePage();
				if (ensureCaretVisible) {
					this._showCaret();
				}
			}
		},
		_updateRuler: function (divRuler, topIndex, bottomIndex) {
			if (!divRuler) { return; }
			var cells = divRuler.firstChild.rows[0].cells;
			var lineHeight = this._getLineHeight();
			var parentDocument = this._frameDocument;
			var viewPad = this._getViewPadding();
			for (var i = 0; i < cells.length; i++) {
				var div = cells[i].firstChild;
				var ruler = div._ruler, style;
				if (div.rulerChanged) {
					this._applyStyle(ruler.getStyle(), div);
				}
				
				var widthDiv;
				var child = div.firstChild;
				if (child) {
					widthDiv = child;
					child = child.nextSibling;
				} else {
					widthDiv = parentDocument.createElement("DIV");
					widthDiv.style.visibility = "hidden";
					div.appendChild(widthDiv);
				}
				var lineIndex;
				if (div.rulerChanged) {
					if (widthDiv) {
						lineIndex = -1;
						this._applyStyle(ruler.getStyle(lineIndex), widthDiv);
						widthDiv.innerHTML = ruler.getHTML(lineIndex);
						widthDiv.lineIndex = lineIndex;
						widthDiv.style.height = (lineHeight + viewPad.top) + "px";
					}
				}

				var overview = ruler.getOverview(), lineDiv, frag;
				if (overview === "page") {
					while (child) {
						lineIndex = child.lineIndex;
						var nextChild = child.nextSibling;
						if (!(topIndex <= lineIndex && lineIndex <= bottomIndex) || child.lineChanged) {
							div.removeChild(child);
						}
						child = nextChild;
					}
					child = div.firstChild.nextSibling;
					frag = document.createDocumentFragment();
					for (lineIndex=topIndex; lineIndex<=bottomIndex; lineIndex++) {
						if (!child || child.lineIndex > lineIndex) {
							lineDiv = parentDocument.createElement("DIV");
							this._applyStyle(ruler.getStyle(lineIndex), lineDiv);
							lineDiv.innerHTML = ruler.getHTML(lineIndex);
							lineDiv.lineIndex = lineIndex;
							lineDiv.style.height = lineHeight + "px";
							frag.appendChild(lineDiv);
						} else {
							if (frag.firstChild) {
								div.insertBefore(frag, child);
								frag = document.createDocumentFragment();
							}
							if (child) {
								child = child.nextSibling;
							}
						}
					}
					if (frag.firstChild) { div.insertBefore(frag, child); }
				} else {
					var buttonHeight = 17;
					var clientHeight = this._getClientHeight ();
					var trackHeight = clientHeight + viewPad.top + viewPad.bottom - 2 * buttonHeight;
					var lineCount = this._model.getLineCount ();
					var divHeight = trackHeight / lineCount;
					if (div.rulerChanged) {
						var count = div.childNodes.length;
						while (count > 1) {
							div.removeChild(div.lastChild);
							count--;
						}
						var lines = ruler.getAnnotations();
						frag = document.createDocumentFragment();
						for (var j = 0; j < lines.length; j++) {
							lineIndex = lines[j];
							lineDiv = parentDocument.createElement("DIV");
							this._applyStyle(ruler.getStyle(lineIndex), lineDiv);
							lineDiv.style.position = "absolute";
							lineDiv.style.top = buttonHeight + lineHeight + Math.floor(lineIndex * divHeight) + "px";
							lineDiv.innerHTML = ruler.getHTML(lineIndex);
							lineDiv.lineIndex = lineIndex;
							frag.appendChild(lineDiv);
						}
						div.appendChild(frag);
					} else if (div._oldTrackHeight !== trackHeight) {
						lineDiv = div.firstChild ? div.firstChild.nextSibling : null;
						while (lineDiv) {
							lineDiv.style.top = buttonHeight + lineHeight + Math.floor(lineDiv.lineIndex * divHeight) + "px";
							lineDiv = lineDiv.nextSibling;
						}
					}
					div._oldTrackHeight = trackHeight;
				}
				div.rulerChanged = false;
				div = div.nextSibling;
			}
		}
	};
	
	return TextView;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define(['orion/textview/textModel', 'orion/textview/keyBinding'], function() {
		return orion.textview;
	});
}













var orion = orion || {};

orion.editor = orion.editor || {};





orion.editor.HtmlGrammar = (function() {
	var _fileTypes = [ "html", "htm" ];
	return {
		




		type: "grammar",
		
		




		fileTypes: _fileTypes,
		
		




		grammar: {
			"comment": "HTML syntax rules",
			"name": "HTML",
			"fileTypes": _fileTypes,
			"scopeName": "source.html",
			"uuid": "3B5C76FB-EBB5-D930-F40C-047D082CE99B",
			"patterns": [
				
				{
					"match": "<!(doctype|DOCTYPE)[^>]+>",
					"name": "entity.name.tag.doctype.html"
				},
				{
					"begin": "<!--",
					"end": "-->",
					"beginCaptures": {
						"0": { "name": "punctuation.definition.comment.html" }
					},
					"endCaptures": {
						"0": { "name": "punctuation.definition.comment.html" }
					},
					"patterns": [
						{
							
							"match": "--",
							"name": "invalid.illegal.badcomment.html"
						}
					],
					"contentName": "comment.block.html"
				},
				{ 
					"match": "<[A-Za-z0-9_\\-:]+(?= ?)",
					"name": "entity.name.tag.html"
				},
				{ "include": "#attrName" },
				{ "include": "#qString" },
				{ "include": "#qqString" },
				
				{ 
					"match": "</[A-Za-z0-9_\\-:]+>",
					"name": "entity.name.tag.html"
				},
				{ 
					"match": ">", 
					"name": "entity.name.tag.html"
				} ],
			"repository": {
				"attrName": { 
					"match": "[A-Za-z\\-:]+(?=\\s*=\\s*['\"])",
					"name": "entity.other.attribute.name.html"
				},
				"qqString": { 
					"match": "(\")[^\"]+(\")",
					"name": "string.quoted.double.html"
				},
				"qString": { 
					"match": "(')[^']+(\')",
					"name": "string.quoted.single.html"
				}
			}
		}
	};
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.editor;
	});
}













var orion = orion || {};
orion.editor = orion.editor || {};







orion.editor.AbstractStyler = (function() {
	
	function AbstractStyler() {
	}
	AbstractStyler.prototype =  {
		




		initialize: function(textView) {
			this.textView = textView;
			
			textView.addEventListener("Selection", this, this._onSelection);
			textView.addEventListener("ModelChanged", this, this._onModelChanged);
			textView.addEventListener("Destroy", this, this._onDestroy);
			textView.addEventListener("LineStyle", this, this._onLineStyle);
			textView.redrawLines();
		},
		
		destroy: function() {
			if (this.textView) {
				this.textView.removeEventListener("Selection", this, this._onSelection);
				this.textView.removeEventListener("ModelChanged", this, this._onModelChanged);
				this.textView.removeEventListener("Destroy", this, this._onDestroy);
				this.textView.removeEventListener("LineStyle", this, this._onLineStyle);
				this.textView = null;
			}
		},
		


		_onSelection: function( selectionEvent) {},
		


		_onModelChanged: function( modelChangedEvent) {},
		


		_onDestroy: function( destroyEvent) {},
		


		_onLineStyle: function( lineStyleEvent) {}
	};
	
	return AbstractStyler;
}());









orion.editor.AbstractStyler.extend = function(subCtor, proto) {
	if (typeof(subCtor) !== "function") { throw new Error("Function expected"); }
	subCtor.prototype = new orion.editor.AbstractStyler();
	subCtor.constructor = subCtor;
	for (var p in proto) {
		if (proto.hasOwnProperty(p)) { subCtor.prototype[p] = proto[p]; }
	}
};

orion.editor.RegexUtil = {
	
	unsupported: [
		{regex: /\(\?[ims\-]:/, func: function(match) { return "option on/off for subexp"; }},
		{regex: /\(\?<([=!])/, func: function(match) { return (match[1] === "=") ? "lookbehind" : "negative lookbehind"; }},
		{regex: /\(\?>/, func: function(match) { return "atomic group"; }}
	],
	
	




	toRegExp: function(str) {
		function fail(feature, match) {
			throw new Error("Unsupported regex feature \"" + feature + "\": \"" + match[0] + "\" at index: "
					+ match.index + " in " + match.input);
		}
		function getMatchingCloseParen(str, start) {
			var depth = 0,
			    len = str.length,
			    xStop = -1;
			for (var i=start; i < len && xStop === -1; i++) {
				switch (str[i]) {
					case "\\":
						i += 1; 
						break;
					case "(":
						depth++;
						break;
					case ")":
						depth--;
						if (depth === 0) {
							xStop = i;
						}
						break;
				}
			}
			return xStop;
		}
		
		function normalize( str) {
			var result = "";
			var insideCharacterClass = false;
			var len = str.length;
			for (var i=0; i < len; ) {
				var chr = str[i];
				if (!insideCharacterClass && chr === "#") {
					
					while (i < len && chr !== "\r" && chr !== "\n") {
						chr = str[++i];
					}
				} else if (!insideCharacterClass && /\s/.test(chr)) {
					
					while (i < len && /\s/.test(chr)) { 
						chr = str[++i];
					}
				} else if (chr === "\\") {
					result += chr;
					if (!/\s/.test(str[i+1])) {
						result += str[i+1];
						i += 1;
					}
					i += 1;
				} else if (chr === "[") {
					insideCharacterClass = true;
					result += chr;
					i += 1;
				} else if (chr === "]") {
					insideCharacterClass = false;
					result += chr;
					i += 1;
				} else {
					result += chr;
					i += 1;
				}
			}
			return result;
		}
		
		var flags = "";
		var i;
		
		
		for (i=0; i < this.unsupported.length; i++) {
			var match;
			if ((match = this.unsupported[i].regex.exec(str))) {
				fail(this.unsupported[i].func(match));
			}
		}
		
		
		if (str.substring(0, 4) === "(?x)") {
			
			str = normalize(str.substring(4));
		} else if (str.substring(0, 4) === "(?x:") {
			
			var xStop = getMatchingCloseParen(str, 0);
			if (xStop < str.length-1) {
				throw new Error("Only a (?x:) group that encloses the entire regex is supported: " + str);
			}
			str = normalize(str.substring(4, xStop));
		}
		
		return new RegExp(str, flags);
	},
	
	hasBackReference: function( regex) {
		return (/\\\d+/).test(regex.source);
	},
	
	

	getSubstitutedRegex: function( regex,  sub,  escape) {
		escape = (typeof escape === "undefined") ? true : false;
		var exploded = regex.source.split(/(\\\d+)/g);
		var array = [];
		for (var i=0; i < exploded.length; i++) {
			var term = exploded[i];
			var backrefMatch = /\\(\d+)/.exec(term);
			if (backrefMatch) {
				var text = sub[backrefMatch[1]] || "";
				array.push(escape ? orion.editor.RegexUtil.escapeRegex(text) : text);
			} else {
				array.push(term);
			}
		}
		return new RegExp(array.join(""));
	},
	
	
	escapeRegex: function( str) {
		return str.replace(/([\\$\^*\/+?\.\(\)|{}\[\]])/g, "\\$&");
	},
	
	















	groupify: function(regex, backRefOld2NewMap) {
		var NON_CAPTURING = 1,
		    CAPTURING = 2,
		    LOOKAHEAD = 3,
		    NEW_CAPTURING = 4;
		var src = regex.source,
		    len = src.length;
		var groups = [],
		    lookaheadDepth = 0,
		    newGroups = [],
		    oldGroupNumber = 1,
		    newGroupNumber = 1;
		var result = [],
		    old2New = {},
		    consuming = {};
		for (var i=0; i < len; i++) {
			var curGroup = groups[groups.length-1];
			var chr = src[i];
			switch (chr) {
				case "(":
					
					if (curGroup === NEW_CAPTURING) {
						groups.pop();
						result.push(")");
						newGroups[newGroups.length-1].end = i;
					}
					var peek2 = (i + 2 < len) ? (src[i+1] + "" + src[i+2]) : null;
					if (peek2 === "?:" || peek2 === "?=" || peek2 === "?!") {
						
						
						
						var groupType;
						if (peek2 === "?:") {
							groupType = NON_CAPTURING;
						} else {
							groupType = LOOKAHEAD;
							lookaheadDepth++;
						}
						groups.push(groupType);
						newGroups.push({ start: i, end: -1, type: groupType  });
						result.push(chr);
						result.push(peek2);
						i += peek2.length;
					} else {
						groups.push(CAPTURING);
						newGroups.push({ start: i, end: -1, type: CAPTURING, oldNum: oldGroupNumber, num: newGroupNumber });
						result.push(chr);
						if (lookaheadDepth === 0) {
							consuming[newGroupNumber] = null;
						}
						old2New[oldGroupNumber] = newGroupNumber;
						oldGroupNumber++;
						newGroupNumber++;
					}
					break;
				case ")":
					var group = groups.pop();
					if (group === LOOKAHEAD) { lookaheadDepth--; }
					newGroups[newGroups.length-1].end = i;
					result.push(chr);
					break;
				case "*":
				case "+":
				case "?":
				case "}":
					
					
					var op = chr;
					var prev = src[i-1],
					    prevIndex = i-1;
					if (chr === "}") {
						for (var j=i-1; src[j] !== "{" && j >= 0; j--) {}
						prev = src[j-1];
						prevIndex = j-1;
						op = src.substring(j, i+1);
					}
					var lastGroup = newGroups[newGroups.length-1];
					if (prev === ")" && (lastGroup.type === CAPTURING || lastGroup.type === NEW_CAPTURING)) {
						
						result.splice(lastGroup.start, 0, "(");
						result.push(op);
						result.push(")");
						var newGroup = { start: lastGroup.start, end: result.length-1, type: NEW_CAPTURING, num: lastGroup.num };
						for (var k=0; k < newGroups.length; k++) {
							group = newGroups[k];
							if (group.type === CAPTURING || group.type === NEW_CAPTURING) {
								if (group.start >= lastGroup.start && group.end <= prevIndex) {
									group.start += 1;
									group.end += 1;
									group.num = group.num + 1;
									if (group.type === CAPTURING) {
										old2New[group.oldNum] = group.num;
									}
								}
							}
						}
						newGroups.push(newGroup);
						newGroupNumber++;
						break;
					} else {
						
					}
				default:
					if (chr !== "|" && curGroup !== CAPTURING && curGroup !== NEW_CAPTURING) {
						
						
						
						if (lookaheadDepth === 0) {
							groups.push(NEW_CAPTURING);
							newGroups.push({ start: i, end: -1, type: NEW_CAPTURING, num: newGroupNumber });
							result.push("(");
							consuming[newGroupNumber] = null;
							newGroupNumber++;
						}
					}
					result.push(chr);
					if (chr === "\\") {
						var peek = src[i+1];
						
						result.push(peek);
						i += 1;
					}
					break;
			}
		}
		while (groups.length) {	
			
			groups.pop();
			result.push(")");
		}
		var newRegex = new RegExp(result.join(""));
		
		
		var subst = {};
		backRefOld2NewMap = backRefOld2NewMap || old2New;
		for (var prop in backRefOld2NewMap) {
			if (backRefOld2NewMap.hasOwnProperty(prop)) {
				subst[prop] = "\\" + backRefOld2NewMap[prop];
			}
		}
		newRegex = this.getSubstitutedRegex(newRegex, subst, false);
		
		return [newRegex, old2New, consuming];
	},
	
	
	complexCaptures: function(capturesObj) {
		if (!capturesObj) { return false; }
		for (var prop in capturesObj) {
			if (capturesObj.hasOwnProperty(prop)) {
				if (prop !== "0") {
					return true;
				}
			}
		}
		return false;
	}
};

































































orion.editor.TextMateStyler = (function() {
	
	function TextMateStyler(textView, grammar) {
		this.initialize(textView);
		
		this.grammar = this.copy(grammar);
		this._styles = {}; 
		this._tree = null;
		
		this.preprocess();
	}
	orion.editor.AbstractStyler.extend(TextMateStyler,  {
		
		copy: function(obj) {
			return JSON.parse(JSON.stringify(obj));
		},
		
		preprocess: function() {
			var stack = [this.grammar];
			for (; stack.length !== 0; ) {
				var rule = stack.pop();
				if (rule._resolvedRule && rule._typedRule) {
					continue;
				}

				
				
				rule._resolvedRule = this._resolve(rule);
				rule._typedRule = this._createTypedRule(rule);
				
				
				this.addStyles(rule.name);
				this.addStyles(rule.contentName);
				this.addStylesForCaptures(rule.captures);
				this.addStylesForCaptures(rule.beginCaptures);
				this.addStylesForCaptures(rule.endCaptures);
				
				if (rule._resolvedRule !== rule) {
					
					stack.push(rule._resolvedRule);
				}
				if (rule.patterns) {
					
					for (var i=0; i < rule.patterns.length; i++) {
						stack.push(rule.patterns[i]);
					}
				}
			}
		},
		
		




		addStyles: function(scope) {
			if (scope && !this._styles[scope]) {
				this._styles[scope] = [];
				var scopeArray = scope.split(".");
				for (var i = 0; i < scopeArray.length; i++) {
					this._styles[scope].push(scopeArray.slice(0, i + 1).join("-"));
				}
			}
		},
		
		addStylesForCaptures: function( captures) {
			for (var prop in captures) {
				if (captures.hasOwnProperty(prop)) {
					var scope = captures[prop].name;
					this.addStyles(scope);
				}
			}
		},
		




		ContainerRule: (function() {
			function ContainerRule( rule) {
				this.rule = rule;
				this.subrules = rule.patterns;
			}
			ContainerRule.prototype.valueOf = function() { return "aa"; };
			return ContainerRule;
		}()),
		




		BeginEndRule: (function() {
			function BeginEndRule( rule) {
				this.rule = rule;
				
				this.beginRegex = orion.editor.RegexUtil.toRegExp(rule.begin);
				this.endRegex = orion.editor.RegexUtil.toRegExp(rule.end);
				this.subrules = rule.patterns || [];
				
				this.endRegexHasBackRef = orion.editor.RegexUtil.hasBackReference(this.endRegex);
				
				
				var complexCaptures = orion.editor.RegexUtil.complexCaptures(rule.captures);
				var complexBeginEnd = orion.editor.RegexUtil.complexCaptures(rule.beginCaptures) || orion.editor.RegexUtil.complexCaptures(rule.endCaptures);
				this.isComplex = complexCaptures || complexBeginEnd;
				if (this.isComplex) {
					var bg = orion.editor.RegexUtil.groupify(this.beginRegex);
					this.beginRegex = bg[0];
					this.beginOld2New = bg[1];
					this.beginConsuming = bg[2];
					
					var eg = orion.editor.RegexUtil.groupify(this.endRegex, this.beginOld2New );
					this.endRegex = eg[0];
					this.endOld2New = eg[1];
					this.endConsuming = eg[2];
				}
			}
			BeginEndRule.prototype.valueOf = function() { return this.beginRegex; };
			return BeginEndRule;
		}()),
		



		MatchRule: (function() {
			function MatchRule( rule) {
				this.rule = rule;
				this.matchRegex = orion.editor.RegexUtil.toRegExp(rule.match);
				this.isComplex = orion.editor.RegexUtil.complexCaptures(rule.captures);
				if (this.isComplex) {
					var mg = orion.editor.RegexUtil.groupify(this.matchRegex);
					this.matchRegex = mg[0];
					this.matchOld2New = mg[1];
					this.matchConsuming = mg[2];
				}
			}
			MatchRule.prototype.valueOf = function() { return this.matchRegex; };
			return MatchRule;
		}()),
		




		_createTypedRule: function(rule) {
			if (rule.match) {
				return new this.MatchRule(rule);
			} else if (rule.begin) {
				return new this.BeginEndRule(rule);
			} else {
				return new this.ContainerRule(rule);
			}
		},
		



		_resolve: function(rule) {
			var resolved = rule;
			if (rule.include) {
				if (rule.begin || rule.end || rule.match) {
					throw new Error("Unexpected regex pattern in \"include\" rule " + rule.include);
				}
				var name = rule.include;
				if (name.charAt(0) === "#") {
					resolved = this.grammar.repository && this.grammar.repository[name.substring(1)];
					if (!resolved) { throw new Error("Couldn't find included rule " + name + " in grammar repository"); }
				} else if (name === "$self") {
					resolved = this.grammar;
				} else if (name === "$base") {
					
					throw new Error("Include \"$base\" is not supported"); 
				} else {
					throw new Error("Include external rule \"" + name + "\" is not supported");
				}
			}
			return resolved;
		},
		
		ContainerNode: (function() {
			function ContainerNode(parent, rule) {
				this.parent = parent;
				this.rule = rule;
				this.children = [];
				
				this.start = null;
				this.end = null;
			}
			ContainerNode.prototype.addChild = function(child) {
				this.children.push(child);
			};
			ContainerNode.prototype.valueOf = function() {
				var r = this.rule;
				return "ContainerNode { " + (r.include || "") + " " + (r.name || "") + (r.comment || "") + "}";
			};
			return ContainerNode;
		}()),
		
		BeginEndNode: (function() {
			function BeginEndNode(parent, rule, beginMatch) {
				this.parent = parent;
				this.rule = rule;
				this.children = [];
				
				this.setStart(beginMatch);
				this.end = null; 
				this.endMatch = null; 
				
				
				if (rule.endRegexHasBackRef) {
					this.endRegexSubstituted = orion.editor.RegexUtil.getSubstitutedRegex(rule.endRegex, beginMatch);
				} else {
					this.endRegexSubstituted = null;
				}
			}
			BeginEndNode.prototype.addChild = function(child) {
				this.children.push(child);
			};
			
			BeginEndNode.prototype.getIndexInParent = function(node) {
				return this.parent ? this.parent.children.indexOf(this) : -1;
			};
			
			BeginEndNode.prototype.setStart = function(beginMatch) {
				this.start = beginMatch.index;
				this.beginMatch = beginMatch;
			};
			
			BeginEndNode.prototype.setEnd = function(endMatchOrLastChar) {
				if (endMatchOrLastChar && typeof(endMatchOrLastChar) === "object") {
					var endMatch = endMatchOrLastChar;
					this.endMatch = endMatch;
					this.end = endMatch.index + endMatch[0].length;
				} else {
					var lastChar = endMatchOrLastChar;
					this.endMatch = null;
					this.end = lastChar;
				}
			};
			BeginEndNode.prototype.shiftStart = function(amount) {
				this.start += amount;
				this.beginMatch.index += amount;
			};
			BeginEndNode.prototype.shiftEnd = function(amount) {
				this.end += amount;
				if (this.endMatch) { this.endMatch.index += amount; }
			};
			BeginEndNode.prototype.valueOf = function() {
				return "{" + this.rule.beginRegex + " range=" + this.start + ".." + this.end + "}";
			};
			return BeginEndNode;
		}()),
		


		push: function( stack,  rules) {
			if (!rules) { return; }
			for (var i = rules.length; i > 0; ) {
				stack.push(rules[--i]);
			}
		},
		




		exec: function( regex,  text,  offset) {
			var match = regex.exec(text);
			if (match) { match.index += offset; }
			regex.lastIndex = 0; 
			return match;
		},
		


		afterMatch: function( match) {
			return match.index + match[0].length;
		},
		



		getEndMatch: function( node,  text,  offset) {
			if (node instanceof this.BeginEndNode) {
				var rule = node.rule;
				var endRegex = node.endRegexSubstituted || rule.endRegex;
				if (!endRegex) { return null; }
				return this.exec(endRegex, text, offset);
			}
			return null;
		},
		



		initialParse: function() {
			var last = this.textView.getModel().getCharCount();
			
			var root = new this.ContainerNode(null, this.grammar._typedRule);
			this._tree = root;
			this.parse(this._tree, false, 0);
		},
		_onModelChanged: function( e) {
			var addedCharCount = e.addedCharCount,
			    addedLineCount = e.addedLineCount,
			    removedCharCount = e.removedCharCount,
			    removedLineCount = e.removedLineCount,
			    start = e.start;
			if (!this._tree) {
				this.initialParse();
			} else {
				var model = this.textView.getModel();
				var charCount = model.getCharCount();
				
				
				
				var rs = model.getLineEnd(model.getLineAtOffset(start) - 1); 
				var fd = this.getFirstDamaged(rs, rs);
				rs = rs === -1 ? 0 : rs;
				var stoppedAt;
				if (fd) {
					
					
					stoppedAt = this.parse(fd, true, rs, start, addedCharCount, removedCharCount);
				} else {
					
					stoppedAt = charCount;
				}
				this.textView.redrawRange(rs, stoppedAt);
			}
		},
		



		getFirstDamaged: function(start, end) {
			
			
			if (start < 0) {
				return this._tree;
			}
			
			var nodes = [this._tree];
			var result = null;
			while (nodes.length) {
				var n = nodes.pop();
				if (!n.parent  || this.isDamaged(n, start, end)) {
					
					
					
					if (n instanceof this.BeginEndNode) {
						result = n;
					}
					
					for (var i=0; i < n.children.length; i++) {
						nodes.push(n.children[i]);
					}
				}
			}
			return result || this._tree;
		},
		


		isDamaged: function( n, start, end) {
			
			return (n.start <= end && n.end > start);
		},
		












		parse: function(origNode, repairing, rs, editStart, addedCharCount, removedCharCount) {
			var model = this.textView.getModel();
			var lastLineStart = model.getLineStart(model.getLineCount() - 1);
			var eof = model.getCharCount();
			var initialExpected = this.getInitialExpected(origNode, rs);
			
			
			var re = -1;
			if (repairing) {
				origNode.repaired = true;
				origNode.endNeedsUpdate = true;
				var lastChild = origNode.children[origNode.children.length-1];
				var delta = addedCharCount - removedCharCount;
				var lastChildLineEnd = lastChild ? model.getLineEnd(model.getLineAtOffset(lastChild.end + delta)) : -1;
				var editLineEnd = model.getLineEnd(model.getLineAtOffset(editStart + removedCharCount));
				re = Math.max(lastChildLineEnd, editLineEnd);
			}
			re = (re === -1) ? eof : re;
			
			var expected = initialExpected;
			var node = origNode;
			var matchedChildOrEnd = false;
			var pos = rs;
			var redrawEnd = -1;
			while (node && (!repairing || (pos < re))) {
				var matchInfo = this.getNextMatch(model, node, pos);
				if (!matchInfo) {
					
					pos = (pos >= lastLineStart) ? eof : model.getLineStart(model.getLineAtOffset(pos) + 1);
				}
				var match = matchInfo && matchInfo.match,
				    rule = matchInfo && matchInfo.rule,
				    isSub = matchInfo && matchInfo.isSub,
				    isEnd = matchInfo && matchInfo.isEnd;
				if (isSub) {
					pos = this.afterMatch(match);
					if (rule instanceof this.BeginEndRule) {
						matchedChildOrEnd = true;
						
						if (repairing && rule === expected.rule && node === expected.parent) {
							
							var foundChild = expected;
							foundChild.setStart(match);
							
							foundChild.repaired = true;
							foundChild.endNeedsUpdate = true;
							node = foundChild; 
							expected = this.getNextExpected(expected, "begin");
						} else {
							if (repairing) {
								
								this.prune(node, expected);
								repairing = false;
							}
							
							
							var subNode = new this.BeginEndNode(node, rule, match);
							node.addChild(subNode);
							node = subNode; 
						}
					} else {
						
					}
				} else if (isEnd || pos === eof) {
					if (node instanceof this.BeginEndNode) {
						if (match) {
							matchedChildOrEnd = true;
							redrawEnd = Math.max(redrawEnd, node.end); 
							node.setEnd(match);
							pos = this.afterMatch(match);
							
							if (repairing && node === expected && node.parent === expected.parent) {
								
								node.repaired = true;
								delete node.endNeedsUpdate;
								expected = this.getNextExpected(expected, "end");
							} else {
								if (repairing) {
									
									this.prune(node, expected);
									repairing = false;
								}
							}
						} else {
							
							node.setEnd(eof);
							delete node.endNeedsUpdate;
						}
					}
					node = node.parent; 
				}
				
				if (repairing && pos >= re && !matchedChildOrEnd) {
					
					this.prune(origNode, initialExpected);
					repairing = false;
				}
			} 
			
			this.removeUnrepairedChildren(origNode, repairing, rs);
			
			
			this.cleanup(repairing, origNode, rs, re, eof, addedCharCount, removedCharCount);
			if (repairing) {
				return Math.max(redrawEnd, pos);
			} else {
				return pos; 
			}
		},
		



		removeUnrepairedChildren: function(node, repairing, start) {
			if (repairing) {
				var children = node.children;
				var removeFrom = -1;
				for (var i=0; i < children.length; i++) {
					var child = children[i];
					if (!child.repaired && this.isDamaged(child, start, Number.MAX_VALUE )) {
						removeFrom = i;
						break;
					}
				}
				if (removeFrom !== -1) {
					node.children.length = removeFrom;
				}
			}
		},
		


		cleanup: function(repairing, origNode, rs, re, eof, addedCharCount, removedCharCount) {
			var i, node, maybeRepairedNodes;
			if (repairing) {
				
				var delta = addedCharCount - removedCharCount;
				
				
				var maybeUnrepairedNodes = this.getIntersecting(re-delta+1, eof);
				maybeRepairedNodes = this.getIntersecting(rs, re);
				
				for (i=0; i < maybeUnrepairedNodes.length; i++) {
					node = maybeUnrepairedNodes[i];
					if (!node.repaired && node instanceof this.BeginEndNode) {
						node.shiftEnd(delta);
						node.shiftStart(delta);
					}
				}
				
				for (i=0; i < maybeRepairedNodes.length; i++) {
					node = maybeRepairedNodes[i];
					if (node.repaired && node.endNeedsUpdate) {
						node.shiftEnd(delta);
					}
					delete node.endNeedsUpdate;
					delete node.repaired;
				}
			} else {
				
				maybeRepairedNodes = this.getIntersecting(rs, re);
				for (i=0; i < maybeRepairedNodes.length; i++) {
					delete maybeRepairedNodes[i].repaired;
				}
			}
		},
		











		getNextMatch: function(model, node, pos, matchRulesOnly) {
			var lineIndex = model.getLineAtOffset(pos);
			var lineEnd = model.getLineEnd(lineIndex);
			var line = model.getText(pos, lineEnd);

			var stack = [],
			    expandedContainers = [],
			    subMatches = [],
			    subrules = [];
			this.push(stack, node.rule.subrules);
			while (stack.length) {
				var next = stack.length ? stack.pop() : null;
				var subrule = next && next._resolvedRule._typedRule;
				if (subrule instanceof this.ContainerRule && expandedContainers.indexOf(subrule) === -1) {
					
					expandedContainers.push(subrule);
					this.push(stack, subrule.subrules);
					continue;
				}
				if (subrule && matchRulesOnly && !(subrule.matchRegex)) {
					continue;
				}
				var subMatch = subrule && this.exec(subrule.matchRegex || subrule.beginRegex, line, pos);
				if (subMatch) {
					subMatches.push(subMatch);
					subrules.push(subrule);
				}
			}

			var bestSub = Number.MAX_VALUE,
			    bestSubIndex = -1;
			for (var i=0; i < subMatches.length; i++) {
				var match = subMatches[i];
				if (match.index < bestSub) {
					bestSub = match.index;
					bestSubIndex = i;
				}
			}
			
			if (!matchRulesOnly) {
				
				
				var activeBENode = node;
				var endMatch = this.getEndMatch(node, line, pos);
				if (endMatch) {
					var doEndLast = activeBENode.rule.applyEndPatternLast;
					var endWins = bestSubIndex === -1 || (endMatch.index < bestSub) || (!doEndLast && endMatch.index === bestSub);
					if (endWins) {
						return {isEnd: true, rule: activeBENode.rule, match: endMatch};
					}
				}
			}
			return bestSubIndex === -1 ? null : {isSub: true, rule: subrules[bestSubIndex], match: subMatches[bestSubIndex]};
		},
		










		getInitialExpected: function(node, rs) {
			
			var i, child;
			if (node === this._tree) {
				
				for (i=0; i < node.children.length; i++) {
					child = node.children[i]; 
					if (child.start >= rs) {
						return child;
					}
				}
			} else if (node instanceof this.BeginEndNode) {
				if (node.endMatch) {
					
					var nodeEnd = node.endMatch.index;
					for (i=0; i < node.children.length; i++) {
						child = node.children[i]; 
						if (child.start >= rs) {
							break;
						}
					}
					if (child && child.start < nodeEnd) {
						return child; 
					}
				} else {
					
				}
			}
			return node; 
		},
		







		getNextExpected: function( expected, event) {
			var node = expected;
			if (event === "begin") {
				var child = node.children[0];
				if (child) {
					return child;
				} else {
					return node;
				}
			} else if (event === "end") {
				var parent = node.parent;
				if (parent) {
					var nextSibling = parent.children[parent.children.indexOf(node) + 1];
					if (nextSibling) {
						return nextSibling;
					} else {
						return parent;
					}
				}
			}
			return null;
		},
		


		prune: function( node,  expected) {
			var expectedAChild = expected.parent === node;
			if (expectedAChild) {
				
				node.children.length = expected.getIndexInParent();
			} else if (node instanceof this.BeginEndNode) {
				
				node.endMatch = null;
				node.end = null;
			}
			
			if (node.parent) {
				node.parent.children.length = node.getIndexInParent() + 1;
			}
		},
		_onLineStyle: function( e) {
			function byStart(r1, r2) {
				return r1.start - r2.start;
			}
			
			if (!this._tree) {
				
				this.initialParse();
			}
			var lineStart = e.lineStart,
			    model = this.textView.getModel(),
			    lineEnd = model.getLineEnd(e.lineIndex);
			
			var rs = model.getLineEnd(model.getLineAtOffset(lineStart) - 1); 
			var node = this.getFirstDamaged(rs, rs);
			
			var scopes = this.getLineScope(model, node, lineStart, lineEnd);
			e.ranges = this.toStyleRanges(scopes);

			e.ranges.sort(byStart);
		},
		


		getLineScope: function(model, node, start, end) {
			var pos = start;
			var expected = this.getInitialExpected(node, start);
			var scopes = [],
			    gaps = [];
			while (node && (pos < end)) {
				var matchInfo = this.getNextMatch(model, node, pos);
				if (!matchInfo) { 
					break; 
				}
				var match = matchInfo && matchInfo.match,
				    rule = matchInfo && matchInfo.rule,
				    isSub = matchInfo && matchInfo.isSub,
				    isEnd = matchInfo && matchInfo.isEnd;
				if (match.index !== pos) {
					
					gaps.push({ start: pos, end: match.index, node: node});
				}
				if (isSub) {
					pos = this.afterMatch(match);
					if (rule instanceof this.BeginEndRule) {
						
						this.addBeginScope(scopes, match, rule);
						node = expected; 
						expected = this.getNextExpected(expected, "begin");
					} else {
						
						this.addMatchScope(scopes, match, rule);
					}
				} else if (isEnd) {
					pos = this.afterMatch(match);
					
					this.addEndScope(scopes, match, rule);
					expected = this.getNextExpected(expected, "end");
					node = node.parent; 
				}
			}
			if (pos < end) {
				gaps.push({ start: pos, end: end, node: node });
			}
			var inherited = this.getInheritedLineScope(gaps, start, end);
			return scopes.concat(inherited);
		},
		
		getInheritedLineScope: function(gaps, start, end) {
			var scopes = [];
			for (var i=0; i < gaps.length; i++) {
				var gap = gaps[i];
				var node = gap.node;
				while (node) {
					
					var rule = node.rule.rule;
					var name = rule.name,
					    contentName = rule.contentName;
					
					var scope = contentName || name;
					if (scope) {
						this.addScopeRange(scopes, gap.start, gap.end, scope);
						break;
					}
					node = node.parent;
				}
			}
			return scopes;
		},
		
		addBeginScope: function(scopes, match, typedRule) {
			var rule = typedRule.rule;
			this.addCapturesScope(scopes, match, (rule.beginCaptures || rule.captures), typedRule.isComplex, typedRule.beginOld2New, typedRule.beginConsuming);
		},
		
		addEndScope: function(scopes, match, typedRule) {
			var rule = typedRule.rule;
			this.addCapturesScope(scopes, match, (rule.endCaptures || rule.captures), typedRule.isComplex, typedRule.endOld2New, typedRule.endConsuming);
		},
		
		addMatchScope: function(scopes, match, typedRule) {
			var rule = typedRule.rule,
			    name = rule.name,
			    captures = rule.captures;
			if (captures) {	
				
				this.addCapturesScope(scopes, match, captures, typedRule.isComplex, typedRule.matchOld2New, typedRule.matchConsuming);
			} else {
				this.addScope(scopes, match, name);
			}
		},
		
		addScope: function(scopes, match, name) {
			if (!name) { return; }
			scopes.push({start: match.index, end: this.afterMatch(match), scope: name });
		},
		
		addScopeRange: function(scopes, start, end, name) {
			if (!name) { return; }
			scopes.push({start: start, end: end, scope: name });
		},
		
		addCapturesScope: function(scopes,  match, captures, isComplex, old2New, consuming) {
			if (!captures) { return; }
			if (!isComplex) {
				this.addScope(scopes, match, captures[0] && captures[0].name);
			} else {
				
				
				
				var newGroupStarts = {1: 0};
				var sum = 0;
				for (var num = 1; match[num] !== undefined; num++) {
					if (consuming[num] !== undefined) {
						sum += match[num].length;
					}
					if (match[num+1] !== undefined) {
						newGroupStarts[num + 1] = sum;
					}
				}
				
				var start = match.index;
				for (var oldGroupNum = 1; captures[oldGroupNum]; oldGroupNum++) {
					var scope = captures[oldGroupNum].name;
					var newGroupNum = old2New[oldGroupNum];
					var groupStart = start + newGroupStarts[newGroupNum];
					
					
					if (typeof match[newGroupNum] !== "undefined") {
						var groupEnd = groupStart + match[newGroupNum].length;
						this.addScopeRange(scopes, groupStart, groupEnd, scope);
					}
				}
			}
		},
		


		getIntersecting: function(start, end) {
			var result = [];
			var nodes = this._tree ? [this._tree] : [];
			while (nodes.length) {
				var n = nodes.pop();
				var visitChildren = false;
				if (n instanceof this.ContainerNode) {
					visitChildren = true;
				} else if (this.isDamaged(n, start, end)) {
					visitChildren = true;
					result.push(n);
				}
				if (visitChildren) {
					var len = n.children.length;



					for (var i=0; i < len; i++) {
						nodes.push(n.children[i]);
					}
				}
			}
			return result.reverse();
		},
		_onSelection: function(e) {
		},
		_onDestroy: function( e) {
			this.grammar = null;
			this._styles = null;
			this._tree = null;
		},
		




		toStyleRanges: function( scopeRanges) {
			var styleRanges = [];
			for (var i=0; i < scopeRanges.length; i++) {
				var scopeRange = scopeRanges[i];
				var classNames = this._styles[scopeRange.scope];
				if (!classNames) { throw new Error("styles not found for " + scopeRange.scope); }
				var classNamesString = classNames.join(" ");
				styleRanges.push({start: scopeRange.start, end: scopeRange.end, style: {styleClass: classNamesString}});

			}
			return styleRanges;
		}
	});
	return TextMateStyler;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return orion.editor;
	});
}













var examples = examples || {};
examples.textview = examples.textview || {};

examples.textview.TextStyler = (function() {

	var JS_KEYWORDS =
		["break", "continue", "do", "for",  "new", "this",  
		 "case", "default", "else", "function", "in", "return", "typeof", "while",
		 "comment", "delete", "export", "if",  "switch", "var", "with",
		 "abstract", "implements", "protected",  "instanceof", "public",
		  "interface", "static", 
		  "synchronized", "false",  "throws", 
		 "final", "null", "transient",  "package", "true", 
		 "goto", "private", "catch", "enum", "throw", "class", "extends", "try", 
		 "const", "finally", "debugger", "super", "undefined"];

	var JAVA_KEYWORDS =
		["abstract",
		 "boolean", "break", "byte",
		 "case", "catch", "char", "class", "continue",
		 "default", "do", "double",
		 "else", "extends",
		 "false", "final", "finally", "float", "for",
		 "if", "implements", "import", "instanceof", "int", "interface",
		 "long",
		 "native", "new", "null",
		 "package", "private", "protected", "public",
		 "return",
		 "short", "static", "super", "switch", "synchronized",
		 "this", "throw", "throws", "transient", "true", "try",
		 "void", "volatile",
		 "while"];

	var CSS_KEYWORDS =
		["color", "text-align", "text-indent", "text-decoration", 
		 "font", "font-style", "font-family", "font-weight", "font-size", "font-variant", "line-height",
		 "background", "background-color", "background-image", "background-position", "background-repeat", "background-attachment",
		 "list-style", "list-style-image", "list-style-position", "list-style-type", 
		 "outline", "outline-color", "outline-style", "outline-width",
		 "border", "border-left", "border-top", "border-bottom", "border-right", "border-color", "border-width", "border-style",
		 "border-bottom-color", "border-bottom-style", "border-bottom-width",
		 "border-left-color", "border-left-style", "border-left-width",
		 "border-top-color", "border-top-style", "border-top-width",
		 "border-right-color", "border-right-style", "border-right-width",
		 "padding", "padding-left", "padding-top", "padding-bottom", "padding-right",
		 "margin", "margin-left", "margin-top", "margin-bottom", "margin-right",
		 "width", "height", "left", "top", "right", "bottom",
		 "min-width", "max-width", "min-height", "max-height",
		 "display", "visibility",
		 "clip", "cursor", "overflow", "overflow-x", "overflow-y", "position", "z-index",
		 "vertical-align", "horizontal-align",
		 "float", "clear"
		];

	
	var UNKOWN = 1;
	var KEYWORD = 2;
	var STRING = 3;
	var COMMENT = 4;
	var WHITE = 5;
	var WHITE_TAB = 6;
	var WHITE_SPACE = 7;

	
	var isIE = document.selection && window.ActiveXObject && /MSIE/.test(navigator.userAgent) ? document.documentMode : undefined;
	var commentStyle = {styleClass: "token_comment"};
	var javadocStyle = {styleClass: "token_javadoc"};
	var stringStyle = {styleClass: "token_string"};
	var keywordStyle = {styleClass: "token_keyword"};
	var spaceStyle = {styleClass: "token_space"};
	var tabStyle = {styleClass: "token_tab"};
	var bracketStyle = {styleClass: isIE < 9 ? "token_bracket" : "token_bracket_outline"};
	var caretLineStyle = {styleClass: "line_caret"};
	
	var Scanner = (function() {
		function Scanner (keywords, whitespacesVisible) {
			this.keywords = keywords;
			this.whitespacesVisible = whitespacesVisible;
			this.setText("");
		}
		
		Scanner.prototype = {
			getOffset: function() {
				return this.offset;
			},
			getStartOffset: function() {
				return this.startOffset;
			},
			getData: function() {
				return this.text.substring(this.startOffset, this.offset);
			},
			getDataLength: function() {
				return this.offset - this.startOffset;
			},
			_read: function() {
				if (this.offset < this.text.length) {
					return this.text.charCodeAt(this.offset++);
				}
				return -1;
			},
			_unread: function(c) {
				if (c !== -1) { this.offset--; }
			},
			nextToken: function() {
				this.startOffset = this.offset;
				while (true) {
					var c = this._read();
					switch (c) {
						case -1: return null;
						case 47:	
							c = this._read();
							if (c === 47) {
								while (true) {
									c = this._read();
									if ((c === -1) || (c === 10)) {
										this._unread(c);
										return COMMENT;
									}
								}
							}
							this._unread(c);
							return UNKOWN;
						case 39:	
							while(true) {
								c = this._read();
								switch (c) {
									case 39:
										return STRING;
									case -1:
										this._unread(c);
										return STRING;
									case 92: 
										c = this._read();
										break;
								}
							}
							break;
						case 34:	
							while(true) {
								c = this._read();
								switch (c) {
									case 34: 
										return STRING;
									case -1:
										this._unread(c);
										return STRING;
									case 92: 
										c = this._read();
										break;
								}
							}
							break;
						case 32: 
						case 9: 
							if (this.whitespacesVisible) {
								return c === 32 ? WHITE_SPACE : WHITE_TAB;
							}
							do {
								c = this._read();
							} while(c === 32 || c === 9);
							this._unread(c);
							return WHITE;
						default:
							var isCSS = this.isCSS;
							if ((97 <= c && c <= 122) || (65 <= c && c <= 90) || c === 95 || (48 <= c && c <= 57) || (0x2d === c && isCSS)) { 
								var off = this.offset - 1;
								do {
									c = this._read();
								} while((97 <= c && c <= 122) || (65 <= c && c <= 90) || c === 95 || (48 <= c && c <= 57) || (0x2d === c && isCSS));  
								this._unread(c);
								var word = this.text.substring(off, this.offset);
								
								for (var i=0; i<this.keywords.length; i++) {
									if (this.keywords[i] === word) { return KEYWORD; }
								}
							}
							return UNKOWN;
					}
				}
			},
			setText: function(text) {
				this.text = text;
				this.offset = 0;
				this.startOffset = 0;
			}
		};
		return Scanner;
	}());
	
	var WhitespaceScanner = (function() {
		function WhitespaceScanner () {
			Scanner.call(this, null, true);
		}
		WhitespaceScanner.prototype = new Scanner(null);
		WhitespaceScanner.prototype.nextToken = function() {
			this.startOffset = this.offset;
			while (true) {
				var c = this._read();
				switch (c) {
					case -1: return null;
					case 32: 
						return WHITE_SPACE;
					case 9: 
						return WHITE_TAB;
					default:
						do {
							c = this._read();
						} while(!(c === 32 || c === 9 || c === -1));
						this._unread(c);
						return UNKOWN;
				}
			}
		};
		
		return WhitespaceScanner;
	}());
	
	function TextStyler (view, lang) {
		this.commentStart = "/*";
		this.commentEnd = "*/";
		var keywords = [];
		switch (lang) {
			case "java": keywords = JAVA_KEYWORDS; break;
			case "js": keywords = JS_KEYWORDS; break;
			case "css": keywords = CSS_KEYWORDS; break;
		}
		this.whitespacesVisible = false;
		this.highlightCaretLine = true;
		this._scanner = new Scanner(keywords, this.whitespacesVisible);
		
		if (lang === "css") {
			this._scanner.isCSS = true;
		}
		this._whitespaceScanner = new WhitespaceScanner();
		this.view = view;
		this.commentOffset = 0;
		this.commentOffsets = [];
		this._currentBracket = undefined; 
		this._matchingBracket = undefined;
		
		view.addEventListener("Selection", this, this._onSelection);
		view.addEventListener("ModelChanged", this, this._onModelChanged);
		view.addEventListener("Destroy", this, this._onDestroy);
		view.addEventListener("LineStyle", this, this._onLineStyle);
		view.redrawLines();
	}
	
	TextStyler.prototype = {
		destroy: function() {
			var view = this.view;
			if (view) {
				view.removeEventListener("Selection", this, this._onSelection);
				view.removeEventListener("ModelChanged", this, this._onModelChanged);
				view.removeEventListener("Destroy", this, this._onDestroy);
				view.removeEventListener("LineStyle", this, this._onLineStyle);
				this.view = null;
			}
		},
		setHighlightCaretLine: function(highlight) {
			this.highlightCaretLine = highlight;
		},
		setWhitespacesVisible: function(visible) {
			this.whitespacesVisible = visible;
			this._scanner.whitespacesVisible = visible;
		},
		_binarySearch: function(offsets, offset, low, high) {
			while (high - low > 2) {
				var index = (((high + low) >> 1) >> 1) << 1;
				var end = offsets[index + 1];
				if (end > offset) {
					high = index;
				} else {
					low = index;
				}
			}
			return high;
		},
		_computeComments: function(end) {
			
			if (end <= this.commentOffset) { return; }
			var model = this.view.getModel();
			var charCount = model.getCharCount();
			var e = end;
			

			var t = model.getText(this.commentOffset, e);
			if (this.commentOffsets.length > 1 && this.commentOffsets[this.commentOffsets.length - 1] === charCount) {
				this.commentOffsets.length--;
			}
			var offset = 0;
			while (offset < t.length) {
				var begin = (this.commentOffsets.length & 1) === 0;
				var search = begin ? this.commentStart : this.commentEnd;
				var index = t.indexOf(search, offset);
				if (index !== -1) {
					this.commentOffsets.push(this.commentOffset + (begin ? index : index + search.length));
				} else {
					break;
				}
				offset = index + search.length;
			}
			if ((this.commentOffsets.length & 1) === 1) { this.commentOffsets.push(charCount); }
			this.commentOffset = e;
		},
		_getCommentRanges: function(start, end) {
			this._computeComments (end);
			var commentCount = this.commentOffsets.length;
			var commentStart = this._binarySearch(this.commentOffsets, start, -1, commentCount);
			if (commentStart >= commentCount) { return []; }
			if (this.commentOffsets[commentStart] > end) { return []; }
			var commentEnd = Math.min(commentCount - 2, this._binarySearch(this.commentOffsets, end, commentStart - 1, commentCount));
			if (this.commentOffsets[commentEnd] > end) { commentEnd = Math.max(commentStart, commentEnd - 2); }
			return this.commentOffsets.slice(commentStart, commentEnd + 2);
		},
		_getLineStyle: function(lineIndex) {
			if (this.highlightCaretLine) {
				var view = this.view;
				var model = view.getModel();
				var selection = view.getSelection();
				if (selection.start === selection.end && model.getLineAtOffset(selection.start) === lineIndex) {
					return caretLineStyle;
				}
			}
			return null;
		},
		_getStyles: function(text, start) {
			var end = start + text.length;
			var model = this.view.getModel();
			
			
			var commentRanges = this._getCommentRanges (start, end);
			var styles = [];
			
			
			var offset = start;
			for (var i = 0; i < commentRanges.length; i+= 2) {
				var commentStart = commentRanges[i];
				if (offset < commentStart) {
					this._parse(text.substring(offset - start, commentStart - start), offset, styles);
				}
				var style = commentStyle;
				if ((commentRanges[i+1] - commentStart) > (this.commentStart.length + this.commentEnd.length)) {
					var o = commentStart + this.commentStart.length;
					if (model.getText(o, o + 1) === "*") { style = javadocStyle; }
				}
				if (this.whitespacesVisible) {
					var s = Math.max(offset, commentStart);
					var e = Math.min(end, commentRanges[i+1]);
					this._parseWhitespace(text.substring(s - start, e - start), s, styles, style);
				} else {
					styles.push({start: commentRanges[i], end: commentRanges[i+1], style: style});
				}
				offset = commentRanges[i+1];
			}
			if (offset < end) {
				this._parse(text.substring(offset - start, end - start), offset, styles);
			}
			return styles;
		},
		_parse: function(text, offset, styles) {
			var scanner = this._scanner;
			scanner.setText(text);
			var token;
			while ((token = scanner.nextToken())) {
				var tokenStart = scanner.getStartOffset() + offset;
				var style = null;
				if (tokenStart === this._matchingBracket) {
					style = bracketStyle;
				} else {
					switch (token) {
						case KEYWORD: style = keywordStyle; break;
						case STRING:
							if (this.whitespacesVisible) {
								this._parseWhitespace(scanner.getData(), tokenStart, styles, stringStyle);
								continue;
							} else {
								style = stringStyle;
							}
							break;
						case COMMENT: 
							if (this.whitespacesVisible) {
								this._parseWhitespace(scanner.getData(), tokenStart, styles, commentStyle);
								continue;
							} else {
								style = commentStyle;
							}
							break;
						case WHITE_TAB:
							if (this.whitespacesVisible) {
								style = tabStyle;
							}
							break;
						case WHITE_SPACE:
							if (this.whitespacesVisible) {
								style = spaceStyle;
							}
							break;
					}
				}
				styles.push({start: tokenStart, end: scanner.getOffset() + offset, style: style});
			}
		},
		_parseWhitespace: function(text, offset, styles, s) {
			var scanner = this._whitespaceScanner;
			scanner.setText(text);
			var token;
			while ((token = scanner.nextToken())) {
				var tokenStart = scanner.getStartOffset() + offset;
				var style = s;
				switch (token) {
					case WHITE_TAB:
						style = tabStyle;
						break;
					case WHITE_SPACE:
						style = spaceStyle;
						break;
				}
				styles.push({start: tokenStart, end: scanner.getOffset() + offset, style: style});
			}
		},
		_findBrackets: function(bracket, closingBracket, text, textOffset, start, end) {
			var result = [];
			
			
			var commentRanges = this._getCommentRanges (start, end);
			
			
			var offset = start, scanner = this._scanner, token, tokenData;
			for (var i = 0; i < commentRanges.length; i+= 2) {
				var commentStart = commentRanges[i];
				if (offset < commentStart) {
					scanner.setText(text.substring(offset - start, commentStart - start));
					while ((token = scanner.nextToken())) {
						if (scanner.getDataLength() !== 1) { continue; }
						tokenData = scanner.getData();
						if (tokenData === bracket) {
							result.push(scanner.getStartOffset() + offset - start + textOffset);
						}
						if (tokenData === closingBracket) {
							result.push(-(scanner.getStartOffset() + offset - start + textOffset));
						}
					}
				}
				offset = commentRanges[i+1];
			}
			if (offset < end) {
				scanner.setText(text.substring(offset - start, end - start));
				while ((token = scanner.nextToken())) {
					if (scanner.getDataLength() !== 1) { continue; }
					tokenData = scanner.getData();
					if (tokenData === bracket) {
						result.push(scanner.getStartOffset() + offset - start + textOffset);
					}
					if (tokenData === closingBracket) {
						result.push(-(scanner.getStartOffset() + offset - start + textOffset));
					}
				}
			}
			return result;
		},
		_onDestroy: function(e) {
			this.destroy();
		},
		_onLineStyle: function (e) {
			e.style = this._getLineStyle(e.lineIndex);
			e.ranges = this._getStyles(e.lineText, e.lineStart);
		},
		_onSelection: function(e) {
			var oldSelection = e.oldValue;
			var newSelection = e.newValue;
			var view = this.view;
			var model = view.getModel();
			var lineIndex;
			if (this._matchingBracket !== undefined) {
				lineIndex = model.getLineAtOffset(this._matchingBracket);
				view.redrawLines(lineIndex, lineIndex + 1);
				this._matchingBracket = this._currentBracket = undefined;
			}
			if (this.highlightCaretLine) {
				var oldLineIndex = model.getLineAtOffset(oldSelection.start);
				lineIndex = model.getLineAtOffset(newSelection.start);
				var newEmpty = newSelection.start === newSelection.end;
				var oldEmpty = oldSelection.start === oldSelection.end;
				if (!(oldLineIndex === lineIndex && oldEmpty && newEmpty)) {
					if (oldEmpty) {
						view.redrawLines(oldLineIndex, oldLineIndex + 1);
					}
					if ((oldLineIndex !== lineIndex || !oldEmpty) && newEmpty) {
						view.redrawLines(lineIndex, lineIndex + 1);
					}
				}
			}
			if (newSelection.start !== newSelection.end || newSelection.start === 0) {
				return;
			}
			var caret = view.getCaretOffset();
			if (caret === 0) { return; }
			var brackets = "{}()[]<>";
			var bracket = model.getText(caret - 1, caret);
			var bracketIndex = brackets.indexOf(bracket, 0);
			if (bracketIndex === -1) { return; }
			var closingBracket;
			if (bracketIndex & 1) {
				closingBracket = brackets.substring(bracketIndex - 1, bracketIndex);
			} else {
				closingBracket = brackets.substring(bracketIndex + 1, bracketIndex + 2);
			}
			lineIndex = model.getLineAtOffset(caret);
			var lineText = model.getLine(lineIndex);
			var lineStart = model.getLineStart(lineIndex);
			var lineEnd = model.getLineEnd(lineIndex);
			brackets = this._findBrackets(bracket, closingBracket, lineText, lineStart, lineStart, lineEnd);
			for (var i=0; i<brackets.length; i++) {
				var sign = brackets[i] >= 0 ? 1 : -1;
				if (brackets[i] * sign === caret - 1) {
					var level = 1;
					this._currentBracket = brackets[i] * sign;
					if (bracketIndex & 1) {
						i--;
						for (; i>=0; i--) {
							sign = brackets[i] >= 0 ? 1 : -1;
							level += sign;
							if (level === 0) {
								this._matchingBracket = brackets[i] * sign;
								view.redrawLines(lineIndex, lineIndex + 1);
								return;
							}
						}
						lineIndex -= 1;
						while (lineIndex >= 0) {
							lineText = model.getLine(lineIndex);
							lineStart = model.getLineStart(lineIndex);
							lineEnd = model.getLineEnd(lineIndex);
							brackets = this._findBrackets(bracket, closingBracket, lineText, lineStart, lineStart, lineEnd);
							for (var j=brackets.length - 1; j>=0; j--) {
								sign = brackets[j] >= 0 ? 1 : -1;
								level += sign;
								if (level === 0) {
									this._matchingBracket = brackets[j] * sign;
									view.redrawLines(lineIndex, lineIndex + 1);
									return;
								}
							}
							lineIndex--;
						}
					} else {
						i++;
						for (; i<brackets.length; i++) {
							sign = brackets[i] >= 0 ? 1 : -1;
							level += sign;
							if (level === 0) {
								this._matchingBracket = brackets[i] * sign;
								view.redrawLines(lineIndex, lineIndex + 1);
								return;
							}
						}
						lineIndex += 1;
						var lineCount = model.getLineCount ();
						while (lineIndex < lineCount) {
							lineText = model.getLine(lineIndex);
							lineStart = model.getLineStart(lineIndex);
							lineEnd = model.getLineEnd(lineIndex);
							brackets = this._findBrackets(bracket, closingBracket, lineText, lineStart, lineStart, lineEnd);
							for (var k=0; k<brackets.length; k++) {
								sign = brackets[k] >= 0 ? 1 : -1;
								level += sign;
								if (level === 0) {
									this._matchingBracket = brackets[k] * sign;
									view.redrawLines(lineIndex, lineIndex + 1);
									return;
								}
							}
							lineIndex++;
						}
					}
					break;
				}
			}
		},
		_onModelChanged: function(e) {
			var start = e.start;
			var removedCharCount = e.removedCharCount;
			var addedCharCount = e.addedCharCount;
			if (this._matchingBracket && start < this._matchingBracket) { this._matchingBracket += addedCharCount + removedCharCount; }
			if (this._currentBracket && start < this._currentBracket) { this._currentBracket += addedCharCount + removedCharCount; }
			if (start >= this.commentOffset) { return; }
			var model = this.view.getModel();
			





			var commentCount = this.commentOffsets.length;
			var extra = Math.max(this.commentStart.length - 1, this.commentEnd.length - 1);
			if (commentCount === 0) {
				this.commentOffset = Math.max(0, start - extra);
				return;
			}
			var charCount = model.getCharCount();
			var oldCharCount = charCount - addedCharCount + removedCharCount;
			var commentStart = this._binarySearch(this.commentOffsets, start, -1, commentCount);
			var end = start + removedCharCount;
			var commentEnd = this._binarySearch(this.commentOffsets, end, commentStart - 1, commentCount);

			var ts;
			if (commentStart > 0) {
				ts = this.commentOffsets[--commentStart];
			} else {
				ts = Math.max(0, Math.min(this.commentOffsets[commentStart], start) - extra);
				--commentStart;
			}
			var te;
			var redrawEnd = charCount;
			if (commentEnd + 1 < this.commentOffsets.length) {
				te = this.commentOffsets[++commentEnd];
				if (end > (te - this.commentEnd.length)) {
					if (commentEnd + 2 < this.commentOffsets.length) { 
						commentEnd += 2;
						te = this.commentOffsets[commentEnd];
						redrawEnd = te + 1;
						if (redrawEnd > start) { redrawEnd += addedCharCount - removedCharCount; }
					} else {
						te = Math.min(oldCharCount, end + extra);
						this.commentOffset = te;
					}
				}
			} else {
				te = Math.min(oldCharCount, end + extra);
				this.commentOffset = te;
				if (commentEnd > 0 && commentEnd === this.commentOffsets.length) {
					commentEnd = this.commentOffsets.length - 1;
				}
			}
			if (ts > start) { ts += addedCharCount - removedCharCount; }
			if (te > start) { te += addedCharCount - removedCharCount; }
			


			if (this.commentOffsets.length > 1 && this.commentOffsets[this.commentOffsets.length - 1] === oldCharCount) {
				this.commentOffsets.length--;
			}
			
			var offset = 0;
			var newComments = [];
			var t = model.getText(ts, te);
			if (this.commentOffset < te) { this.commentOffset = te; }
			while (offset < t.length) {
				var begin = ((commentStart + 1 + newComments.length) & 1) === 0;
				var search = begin ? this.commentStart : this.commentEnd;
				var index = t.indexOf(search, offset);
				if (index !== -1) {
					newComments.push(ts + (begin ? index : index + search.length));
				} else {
					break;
				}
				offset = index + search.length;
			}




			var redraw = (commentEnd - commentStart) !== newComments.length;
			if (!redraw) {
				for (var i=0; i<newComments.length; i++) {
					offset = this.commentOffsets[commentStart + 1 + i];
					if (offset > start) { offset += addedCharCount - removedCharCount; }
					if (offset !== newComments[i]) {
						redraw = true;
						break;
					} 
				}
			}
			
			var args = [commentStart + 1, (commentEnd - commentStart)].concat(newComments);
			Array.prototype.splice.apply(this.commentOffsets, args);
			for (var k=commentStart + 1 + newComments.length; k< this.commentOffsets.length; k++) {
				this.commentOffsets[k] += addedCharCount - removedCharCount;
			}
			
			if ((this.commentOffsets.length & 1) === 1) { this.commentOffsets.push(charCount); }
			
			if (redraw) {

				this.view.redrawRange(start + addedCharCount, redrawEnd);
			}





		}
	};
	return TextStyler;
}());

if (typeof window !== "undefined" && typeof window.define !== "undefined") {
	define([], function() {
		return examples.textview;
	});
}
