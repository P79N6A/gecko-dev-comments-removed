









MochiKit.Base._deps('Style', ['Base', 'DOM']);

MochiKit.Style.NAME = 'MochiKit.Style';
MochiKit.Style.VERSION = '1.4.2';
MochiKit.Style.__repr__ = function () {
    return '[' + this.NAME + ' ' + this.VERSION + ']';
};
MochiKit.Style.toString = function () {
    return this.__repr__();
};

MochiKit.Style.EXPORT_OK = [];

MochiKit.Style.EXPORT = [
    'setStyle',
    'setOpacity',
    'getStyle',
    'getElementDimensions',
    'elementDimensions', 
    'setElementDimensions',
    'getElementPosition',
    'elementPosition', 
    'setElementPosition',
    "makePositioned",
    "undoPositioned",
    "makeClipping",
    "undoClipping",
    'setDisplayForElement',
    'hideElement',
    'showElement',
    'getViewportDimensions',
    'getViewportPosition',
    'Dimensions',
    'Coordinates'
];








MochiKit.Style.Dimensions = function (w, h) {
    this.w = w;
    this.h = h;
};

MochiKit.Style.Dimensions.prototype.__repr__ = function () {
    var repr = MochiKit.Base.repr;
    return '{w: '  + repr(this.w) + ', h: ' + repr(this.h) + '}';
};

MochiKit.Style.Dimensions.prototype.toString = function () {
    return this.__repr__();
};








MochiKit.Style.Coordinates = function (x, y) {
    this.x = x;
    this.y = y;
};

MochiKit.Style.Coordinates.prototype.__repr__ = function () {
    var repr = MochiKit.Base.repr;
    return '{x: '  + repr(this.x) + ', y: ' + repr(this.y) + '}';
};

MochiKit.Style.Coordinates.prototype.toString = function () {
    return this.__repr__();
};


MochiKit.Base.update(MochiKit.Style, {

    
    getStyle: function (elem, cssProperty) {
        var dom = MochiKit.DOM;
        var d = dom._document;

        elem = dom.getElement(elem);
        cssProperty = MochiKit.Base.camelize(cssProperty);

        if (!elem || elem == d) {
            return undefined;
        }
        if (cssProperty == 'opacity' && typeof(elem.filters) != 'undefined') {
            var opacity = (MochiKit.Style.getStyle(elem, 'filter') || '').match(/alpha\(opacity=(.*)\)/);
            if (opacity && opacity[1]) {
                return parseFloat(opacity[1]) / 100;
            }
            return 1.0;
        }
        if (cssProperty == 'float' || cssProperty == 'cssFloat' || cssProperty == 'styleFloat') {
            if (elem.style["float"]) {
                return elem.style["float"];
            } else if (elem.style.cssFloat) {
                return elem.style.cssFloat;
            } else if (elem.style.styleFloat) {
                return elem.style.styleFloat;
            } else {
                return "none";
            }
        }
        var value = elem.style ? elem.style[cssProperty] : null;
        if (!value) {
            if (d.defaultView && d.defaultView.getComputedStyle) {
                var css = d.defaultView.getComputedStyle(elem, null);
                cssProperty = cssProperty.replace(/([A-Z])/g, '-$1'
                    ).toLowerCase(); 
                value = css ? css.getPropertyValue(cssProperty) : null;
            } else if (elem.currentStyle) {
                value = elem.currentStyle[cssProperty];
                if (/^\d/.test(value) && !/px$/.test(value) && cssProperty != 'fontWeight') {
                    
                    var left = elem.style.left;
                    var rsLeft = elem.runtimeStyle.left;
                    elem.runtimeStyle.left = elem.currentStyle.left;
                    elem.style.left = value || 0;
                    value = elem.style.pixelLeft + "px";
                    elem.style.left = left;
                    elem.runtimeStyle.left = rsLeft;
                }
            }
        }
        if (cssProperty == 'opacity') {
            value = parseFloat(value);
        }

        if (/Opera/.test(navigator.userAgent) && (MochiKit.Base.findValue(['left', 'top', 'right', 'bottom'], cssProperty) != -1)) {
            if (MochiKit.Style.getStyle(elem, 'position') == 'static') {
                value = 'auto';
            }
        }

        return value == 'auto' ? null : value;
    },

    
    setStyle: function (elem, style) {
        elem = MochiKit.DOM.getElement(elem);
        for (var name in style) {
            switch (name) {
            case 'opacity':
                MochiKit.Style.setOpacity(elem, style[name]);
                break;
            case 'float':
            case 'cssFloat':
            case 'styleFloat':
                if (typeof(elem.style["float"]) != "undefined") {
                    elem.style["float"] = style[name];
                } else if (typeof(elem.style.cssFloat) != "undefined") {
                    elem.style.cssFloat = style[name];
                } else {
                    elem.style.styleFloat = style[name];
                }
                break;
            default:
                elem.style[MochiKit.Base.camelize(name)] = style[name];
            }
        }
    },

    
    setOpacity: function (elem, o) {
        elem = MochiKit.DOM.getElement(elem);
        var self = MochiKit.Style;
        if (o == 1) {
            var toSet = /Gecko/.test(navigator.userAgent) && !(/Konqueror|AppleWebKit|KHTML/.test(navigator.userAgent));
            elem.style["opacity"] = toSet ? 0.999999 : 1.0;
            if (/MSIE/.test(navigator.userAgent)) {
                elem.style['filter'] =
                    self.getStyle(elem, 'filter').replace(/alpha\([^\)]*\)/gi, '');
            }
        } else {
            if (o < 0.00001) {
                o = 0;
            }
            elem.style["opacity"] = o;
            if (/MSIE/.test(navigator.userAgent)) {
                elem.style['filter'] =
                    self.getStyle(elem, 'filter').replace(/alpha\([^\)]*\)/gi, '') + 'alpha(opacity=' + o * 100 + ')';
            }
        }
    },

    







    
    getElementPosition: function (elem, relativeTo) {
        var self = MochiKit.Style;
        var dom = MochiKit.DOM;
        elem = dom.getElement(elem);

        if (!elem ||
            (!(elem.x && elem.y) &&
            (!elem.parentNode === null ||
            self.getStyle(elem, 'display') == 'none'))) {
            return undefined;
        }

        var c = new self.Coordinates(0, 0);
        var box = null;
        var parent = null;

        var d = MochiKit.DOM._document;
        var de = d.documentElement;
        var b = d.body;

        if (!elem.parentNode && elem.x && elem.y) {
            
            c.x += elem.x || 0;
            c.y += elem.y || 0;
        } else if (elem.getBoundingClientRect) { 
            








            box = elem.getBoundingClientRect();

            c.x += box.left +
                (de.scrollLeft || b.scrollLeft) -
                (de.clientLeft || 0);

            c.y += box.top +
                (de.scrollTop || b.scrollTop) -
                (de.clientTop || 0);

        } else if (elem.offsetParent) {
            c.x += elem.offsetLeft;
            c.y += elem.offsetTop;
            parent = elem.offsetParent;

            if (parent != elem) {
                while (parent) {
                    c.x += parseInt(parent.style.borderLeftWidth) || 0;
                    c.y += parseInt(parent.style.borderTopWidth) || 0;
                    c.x += parent.offsetLeft;
                    c.y += parent.offsetTop;
                    parent = parent.offsetParent;
                }
            }

            





            var ua = navigator.userAgent.toLowerCase();
            if ((typeof(opera) != 'undefined' &&
                parseFloat(opera.version()) < 9) ||
                (ua.indexOf('AppleWebKit') != -1 &&
                self.getStyle(elem, 'position') == 'absolute')) {

                c.x -= b.offsetLeft;
                c.y -= b.offsetTop;

            }

            
            if (elem.parentNode) {
                parent = elem.parentNode;
            } else {
                parent = null;
            }
            while (parent) {
                var tagName = parent.tagName.toUpperCase();
                if (tagName === 'BODY' || tagName === 'HTML') {
                    break;
                }
                var disp = self.getStyle(parent, 'display');
                
                if (disp.search(/^inline|table-row.*$/i)) {
                    c.x -= parent.scrollLeft;
                    c.y -= parent.scrollTop;
                }
                if (parent.parentNode) {
                    parent = parent.parentNode;
                } else {
                    parent = null;
                }
            }
        }

        if (typeof(relativeTo) != 'undefined') {
            relativeTo = arguments.callee(relativeTo);
            if (relativeTo) {
                c.x -= (relativeTo.x || 0);
                c.y -= (relativeTo.y || 0);
            }
        }

        return c;
    },

    
    setElementPosition: function (elem, newPos, units) {
        elem = MochiKit.DOM.getElement(elem);
        if (typeof(units) == 'undefined') {
            units = 'px';
        }
        var newStyle = {};
        var isUndefNull = MochiKit.Base.isUndefinedOrNull;
        if (!isUndefNull(newPos.x)) {
            newStyle['left'] = newPos.x + units;
        }
        if (!isUndefNull(newPos.y)) {
            newStyle['top'] = newPos.y + units;
        }
        MochiKit.DOM.updateNodeAttributes(elem, {'style': newStyle});
    },

    
    makePositioned: function (element) {
        element = MochiKit.DOM.getElement(element);
        var pos = MochiKit.Style.getStyle(element, 'position');
        if (pos == 'static' || !pos) {
            element.style.position = 'relative';
            
            
            
            if (/Opera/.test(navigator.userAgent)) {
                element.style.top = 0;
                element.style.left = 0;
            }
        }
    },

    
    undoPositioned: function (element) {
        element = MochiKit.DOM.getElement(element);
        if (element.style.position == 'relative') {
            element.style.position = element.style.top = element.style.left = element.style.bottom = element.style.right = '';
        }
    },

    
    makeClipping: function (element) {
        element = MochiKit.DOM.getElement(element);
        var s = element.style;
        var oldOverflow = { 'overflow': s.overflow,
                            'overflow-x': s.overflowX,
                            'overflow-y': s.overflowY };
        if ((MochiKit.Style.getStyle(element, 'overflow') || 'visible') != 'hidden') {
            element.style.overflow = 'hidden';
            element.style.overflowX = 'hidden';
            element.style.overflowY = 'hidden';
        }
        return oldOverflow;
    },

    
    undoClipping: function (element, overflow) {
        element = MochiKit.DOM.getElement(element);
        if (typeof(overflow) == 'string') {
            element.style.overflow = overflow;
        } else if (overflow != null) {
            element.style.overflow = overflow['overflow'];
            element.style.overflowX = overflow['overflow-x'];
            element.style.overflowY = overflow['overflow-y'];
        }
    },

    
    getElementDimensions: function (elem, contentSize) {
        var self = MochiKit.Style;
        var dom = MochiKit.DOM;
        if (typeof(elem.w) == 'number' || typeof(elem.h) == 'number') {
            return new self.Dimensions(elem.w || 0, elem.h || 0);
        }
        elem = dom.getElement(elem);
        if (!elem) {
            return undefined;
        }
        var disp = self.getStyle(elem, 'display');
        
        if (disp == 'none' || disp == '' || typeof(disp) == 'undefined') {
            var s = elem.style;
            var originalVisibility = s.visibility;
            var originalPosition = s.position;
            var originalDisplay = s.display;
            s.visibility = 'hidden';
            s.position = 'absolute';
            s.display = self._getDefaultDisplay(elem);
            var originalWidth = elem.offsetWidth;
            var originalHeight = elem.offsetHeight;
            s.display = originalDisplay;
            s.position = originalPosition;
            s.visibility = originalVisibility;
        } else {
            originalWidth = elem.offsetWidth || 0;
            originalHeight = elem.offsetHeight || 0;
        }
        if (contentSize) {
            var tableCell = 'colSpan' in elem && 'rowSpan' in elem;
            var collapse = (tableCell && elem.parentNode && self.getStyle(
                    elem.parentNode, 'borderCollapse') == 'collapse')
            if (collapse) {
                if (/MSIE/.test(navigator.userAgent)) {
                    var borderLeftQuota = elem.previousSibling? 0.5 : 1;
                    var borderRightQuota = elem.nextSibling? 0.5 : 1;
                }
                else {
                    var borderLeftQuota = 0.5;
                    var borderRightQuota = 0.5;
                }
            } else {
                var borderLeftQuota = 1;
                var borderRightQuota = 1;
            }
            originalWidth -= Math.round(
                (parseFloat(self.getStyle(elem, 'paddingLeft')) || 0)
              + (parseFloat(self.getStyle(elem, 'paddingRight')) || 0)
              + borderLeftQuota *
                (parseFloat(self.getStyle(elem, 'borderLeftWidth')) || 0)
              + borderRightQuota *
                (parseFloat(self.getStyle(elem, 'borderRightWidth')) || 0)
            );
            if (tableCell) {
                if (/Gecko|Opera/.test(navigator.userAgent)
                    && !/Konqueror|AppleWebKit|KHTML/.test(navigator.userAgent)) {
                    var borderHeightQuota = 0;
                } else if (/MSIE/.test(navigator.userAgent)) {
                    var borderHeightQuota = 1;
                } else {
                    var borderHeightQuota = collapse? 0.5 : 1;
                }
            } else {
                var borderHeightQuota = 1;
            }
            originalHeight -= Math.round(
                (parseFloat(self.getStyle(elem, 'paddingTop')) || 0)
              + (parseFloat(self.getStyle(elem, 'paddingBottom')) || 0)
              + borderHeightQuota * (
                (parseFloat(self.getStyle(elem, 'borderTopWidth')) || 0)
              + (parseFloat(self.getStyle(elem, 'borderBottomWidth')) || 0))
            );
        }
        return new self.Dimensions(originalWidth, originalHeight);
    },

    
    setElementDimensions: function (elem, newSize, units) {
        elem = MochiKit.DOM.getElement(elem);
        if (typeof(units) == 'undefined') {
            units = 'px';
        }
        var newStyle = {};
        var isUndefNull = MochiKit.Base.isUndefinedOrNull;
        if (!isUndefNull(newSize.w)) {
            newStyle['width'] = newSize.w + units;
        }
        if (!isUndefNull(newSize.h)) {
            newStyle['height'] = newSize.h + units;
        }
        MochiKit.DOM.updateNodeAttributes(elem, {'style': newStyle});
    },

    _getDefaultDisplay: function (elem) {
        var self = MochiKit.Style;
        var dom = MochiKit.DOM;
        elem = dom.getElement(elem);
        if (!elem) {
            return undefined;
        }
        var tagName = elem.tagName.toUpperCase();
        return self._defaultDisplay[tagName] || 'block';
    },

    
    setDisplayForElement: function (display, element) {
        var elements = MochiKit.Base.extend(null, arguments, 1);
        var getElement = MochiKit.DOM.getElement;
        for (var i = 0; i < elements.length; i++) {
            element = getElement(elements[i]);
            if (element) {
                element.style.display = display;
            }
        }
    },

    
    getViewportDimensions: function () {
        var d = new MochiKit.Style.Dimensions();
        var w = MochiKit.DOM._window;
        var b = MochiKit.DOM._document.body;
        if (w.innerWidth) {
            d.w = w.innerWidth;
            d.h = w.innerHeight;
        } else if (b && b.parentElement && b.parentElement.clientWidth) {
            d.w = b.parentElement.clientWidth;
            d.h = b.parentElement.clientHeight;
        } else if (b && b.clientWidth) {
            d.w = b.clientWidth;
            d.h = b.clientHeight;
        }
        return d;
    },

    
    getViewportPosition: function () {
        var c = new MochiKit.Style.Coordinates(0, 0);
        var d = MochiKit.DOM._document;
        var de = d.documentElement;
        var db = d.body;
        if (de && (de.scrollTop || de.scrollLeft)) {
            c.x = de.scrollLeft;
            c.y = de.scrollTop;
        } else if (db) {
            c.x = db.scrollLeft;
            c.y = db.scrollTop;
        }
        return c;
    },

    __new__: function () {
        var m = MochiKit.Base;

        var inlines = ['A','ABBR','ACRONYM','B','BASEFONT','BDO','BIG','BR',
                       'CITE','CODE','DFN','EM','FONT','I','IMG','KBD','LABEL',
                       'Q','S','SAMP','SMALL','SPAN','STRIKE','STRONG','SUB',
                       'SUP','TEXTAREA','TT','U','VAR'];
        this._defaultDisplay = { 'TABLE': 'table',
                                 'THEAD': 'table-header-group',
                                 'TBODY': 'table-row-group',
                                 'TFOOT': 'table-footer-group',
                                 'COLGROUP': 'table-column-group',
                                 'COL': 'table-column',
                                 'TR': 'table-row',
                                 'TD': 'table-cell',
                                 'TH': 'table-cell',
                                 'CAPTION': 'table-caption',
                                 'LI': 'list-item',
                                 'INPUT': 'inline-block',
                                 'SELECT': 'inline-block' };
        
        if (/MSIE/.test(navigator.userAgent)) {
            for (var k in this._defaultDisplay) {
                var v = this._defaultDisplay[k];
                if (v.indexOf('table') == 0) {
                    this._defaultDisplay[k] = 'block';
                }
            }
        }
        for (var i = 0; i < inlines.length; i++) {
            this._defaultDisplay[inlines[i]] = 'inline';
        }

        this.elementPosition = this.getElementPosition;
        this.elementDimensions = this.getElementDimensions;

        this.hideElement = m.partial(this.setDisplayForElement, 'none');
        
        this.showElement = m.partial(this.setDisplayForElement, 'block');

        this.EXPORT_TAGS = {
            ':common': this.EXPORT,
            ':all': m.concat(this.EXPORT, this.EXPORT_OK)
        };

        m.nameFunctions(this);
    }
});

MochiKit.Style.__new__();
MochiKit.Base._exportSymbols(this, MochiKit.Style);
