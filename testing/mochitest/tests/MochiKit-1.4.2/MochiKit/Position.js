









MochiKit.Base._deps('Position', ['Base', 'DOM', 'Style']);

MochiKit.Position.NAME = 'MochiKit.Position';
MochiKit.Position.VERSION = '1.4.2';
MochiKit.Position.__repr__ = function () {
    return '[' + this.NAME + ' ' + this.VERSION + ']';
};
MochiKit.Position.toString = function () {
    return this.__repr__();
};

MochiKit.Position.EXPORT_OK = [];

MochiKit.Position.EXPORT = [
];


MochiKit.Base.update(MochiKit.Position, {
    
    
    
    includeScrollOffsets: false,

    
    prepare: function () {
        var deltaX =  window.pageXOffset
                   || document.documentElement.scrollLeft
                   || document.body.scrollLeft
                   || 0;
        var deltaY =  window.pageYOffset
                   || document.documentElement.scrollTop
                   || document.body.scrollTop
                   || 0;
        this.windowOffset = new MochiKit.Style.Coordinates(deltaX, deltaY);
    },

    
    cumulativeOffset: function (element) {
        var valueT = 0;
        var valueL = 0;
        do {
            valueT += element.offsetTop  || 0;
            valueL += element.offsetLeft || 0;
            element = element.offsetParent;
        } while (element);
        return new MochiKit.Style.Coordinates(valueL, valueT);
    },

    
    realOffset: function (element) {
        var valueT = 0;
        var valueL = 0;
        do {
            valueT += element.scrollTop  || 0;
            valueL += element.scrollLeft || 0;
            element = element.parentNode;
        } while (element);
        return new MochiKit.Style.Coordinates(valueL, valueT);
    },

    
    within: function (element, x, y) {
        if (this.includeScrollOffsets) {
            return this.withinIncludingScrolloffsets(element, x, y);
        }
        this.xcomp = x;
        this.ycomp = y;
        this.offset = this.cumulativeOffset(element);
        if (element.style.position == "fixed") {
            this.offset.x += this.windowOffset.x;
            this.offset.y += this.windowOffset.y;
        }

        return (y >= this.offset.y &&
                y <  this.offset.y + element.offsetHeight &&
                x >= this.offset.x &&
                x <  this.offset.x + element.offsetWidth);
    },

    
    withinIncludingScrolloffsets: function (element, x, y) {
        var offsetcache = this.realOffset(element);

        this.xcomp = x + offsetcache.x - this.windowOffset.x;
        this.ycomp = y + offsetcache.y - this.windowOffset.y;
        this.offset = this.cumulativeOffset(element);

        return (this.ycomp >= this.offset.y &&
                this.ycomp <  this.offset.y + element.offsetHeight &&
                this.xcomp >= this.offset.x &&
                this.xcomp <  this.offset.x + element.offsetWidth);
    },

    
    
    overlap: function (mode, element) {
        if (!mode) {
            return 0;
        }
        if (mode == 'vertical') {
          return ((this.offset.y + element.offsetHeight) - this.ycomp) /
                 element.offsetHeight;
        }
        if (mode == 'horizontal') {
          return ((this.offset.x + element.offsetWidth) - this.xcomp) /
                 element.offsetWidth;
        }
    },

    
    absolutize: function (element) {
        element = MochiKit.DOM.getElement(element);
        if (element.style.position == 'absolute') {
            return;
        }
        MochiKit.Position.prepare();

        var offsets = MochiKit.Position.positionedOffset(element);
        var width = element.clientWidth;
        var height = element.clientHeight;

        var oldStyle = {
            'position': element.style.position,
            'left': offsets.x - parseFloat(element.style.left  || 0),
            'top': offsets.y - parseFloat(element.style.top || 0),
            'width': element.style.width,
            'height': element.style.height
        };

        element.style.position = 'absolute';
        element.style.top = offsets.y + 'px';
        element.style.left = offsets.x + 'px';
        element.style.width = width + 'px';
        element.style.height = height + 'px';

        return oldStyle;
    },

    
    positionedOffset: function (element) {
        var valueT = 0, valueL = 0;
        do {
            valueT += element.offsetTop  || 0;
            valueL += element.offsetLeft || 0;
            element = element.offsetParent;
            if (element) {
                p = MochiKit.Style.getStyle(element, 'position');
                if (p == 'relative' || p == 'absolute') {
                    break;
                }
            }
        } while (element);
        return new MochiKit.Style.Coordinates(valueL, valueT);
    },

    
    relativize: function (element, oldPos) {
        element = MochiKit.DOM.getElement(element);
        if (element.style.position == 'relative') {
            return;
        }
        MochiKit.Position.prepare();

        var top = parseFloat(element.style.top || 0) -
                  (oldPos['top'] || 0);
        var left = parseFloat(element.style.left || 0) -
                   (oldPos['left'] || 0);

        element.style.position = oldPos['position'];
        element.style.top = top + 'px';
        element.style.left = left + 'px';
        element.style.width = oldPos['width'];
        element.style.height = oldPos['height'];
    },

    
    clone: function (source, target) {
        source = MochiKit.DOM.getElement(source);
        target = MochiKit.DOM.getElement(target);
        target.style.position = 'absolute';
        var offsets = this.cumulativeOffset(source);
        target.style.top = offsets.y + 'px';
        target.style.left = offsets.x + 'px';
        target.style.width = source.offsetWidth + 'px';
        target.style.height = source.offsetHeight + 'px';
    },

    
    page: function (forElement) {
        var valueT = 0;
        var valueL = 0;

        var element = forElement;
        do {
            valueT += element.offsetTop  || 0;
            valueL += element.offsetLeft || 0;

            
            if (element.offsetParent == document.body && MochiKit.Style.getStyle(element, 'position') == 'absolute') {
                break;
            }
        } while (element = element.offsetParent);

        element = forElement;
        do {
            valueT -= element.scrollTop  || 0;
            valueL -= element.scrollLeft || 0;
        } while (element = element.parentNode);

        return new MochiKit.Style.Coordinates(valueL, valueT);
    }
});

MochiKit.Position.__new__ = function (win) {
    var m = MochiKit.Base;
    this.EXPORT_TAGS = {
        ':common': this.EXPORT,
        ':all': m.concat(this.EXPORT, this.EXPORT_OK)
    };

    m.nameFunctions(this);
};

MochiKit.Position.__new__(this);

MochiKit.Base._exportSymbols(this, MochiKit.Position); 