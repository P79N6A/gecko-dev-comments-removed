































































(function ($) {
    function init(plot) {
        var selection = {
                first: { x: -1, y: -1}, second: { x: -1, y: -1},
                show: false,
                active: false
            };

        
        
        
        
        
        var savedhandlers = {};

        function onMouseMove(e) {
            if (selection.active) {
                plot.getPlaceholder().trigger("plotselecting", [ getSelection() ]);

                updateSelection(e);
            }
        }

        function onMouseDown(e) {
            if (e.which != 1)  
                return;
            
            
            document.body.focus();

            
            if (document.onselectstart !== undefined && savedhandlers.onselectstart == null) {
                savedhandlers.onselectstart = document.onselectstart;
                document.onselectstart = function () { return false; };
            }
            if (document.ondrag !== undefined && savedhandlers.ondrag == null) {
                savedhandlers.ondrag = document.ondrag;
                document.ondrag = function () { return false; };
            }

            setSelectionPos(selection.first, e);

            selection.active = true;
            
            $(document).one("mouseup", onMouseUp);
        }

        function onMouseUp(e) {
            
            if (document.onselectstart !== undefined)
                document.onselectstart = savedhandlers.onselectstart;
            if (document.ondrag !== undefined)
                document.ondrag = savedhandlers.ondrag;

            
            selection.active = false;
            updateSelection(e);

            if (selectionIsSane())
                triggerSelectedEvent();
            else {
                
                plot.getPlaceholder().trigger("plotunselected", [ ]);
                plot.getPlaceholder().trigger("plotselecting", [ null ]);
            }

            return false;
        }

        function getSelection() {
            if (!selectionIsSane())
                return null;

            var x1 = Math.min(selection.first.x, selection.second.x),
                x2 = Math.max(selection.first.x, selection.second.x),
                y1 = Math.max(selection.first.y, selection.second.y),
                y2 = Math.min(selection.first.y, selection.second.y);

            var r = {};
            var axes = plot.getAxes();
            if (axes.xaxis.used)
                r.xaxis = { from: axes.xaxis.c2p(x1), to: axes.xaxis.c2p(x2) };
            if (axes.x2axis.used)
                r.x2axis = { from: axes.x2axis.c2p(x1), to: axes.x2axis.c2p(x2) };
            if (axes.yaxis.used)
                r.yaxis = { from: axes.yaxis.c2p(y1), to: axes.yaxis.c2p(y2) };
            if (axes.y2axis.used)
                r.y2axis = { from: axes.y2axis.c2p(y1), to: axes.y2axis.c2p(y2) };
            return r;
        }

        function triggerSelectedEvent() {
            var r = getSelection();

            plot.getPlaceholder().trigger("plotselected", [ r ]);

            
            var axes = plot.getAxes();
            if (axes.xaxis.used && axes.yaxis.used)
                plot.getPlaceholder().trigger("selected", [ { x1: r.xaxis.from, y1: r.yaxis.from, x2: r.xaxis.to, y2: r.yaxis.to } ]);
        }

        function clamp(min, value, max) {
            return value < min? min: (value > max? max: value);
        }

        function setSelectionPos(pos, e) {
            var o = plot.getOptions();
            var offset = plot.getPlaceholder().offset();
            var plotOffset = plot.getPlotOffset();
            pos.x = clamp(0, e.pageX - offset.left - plotOffset.left, plot.width());
            pos.y = clamp(0, e.pageY - offset.top - plotOffset.top, plot.height());

            if (o.selection.mode == "y")
                pos.x = pos == selection.first? 0: plot.width();

            if (o.selection.mode == "x")
                pos.y = pos == selection.first? 0: plot.height();
        }

        function updateSelection(pos) {
            if (pos.pageX == null)
                return;

            setSelectionPos(selection.second, pos);
            if (selectionIsSane()) {
                selection.show = true;
                plot.triggerRedrawOverlay();
            }
            else
                clearSelection(true);
        }

        function clearSelection(preventEvent) {
            if (selection.show) {
                selection.show = false;
                plot.triggerRedrawOverlay();
                if (!preventEvent)
                    plot.getPlaceholder().trigger("plotunselected", [ ]);
            }
        }

        function setSelection(ranges, preventEvent) {
            var axis, range, axes = plot.getAxes();
            var o = plot.getOptions();

            if (o.selection.mode == "y") {
                selection.first.x = 0;
                selection.second.x = plot.width();
            }
            else {
                axis = ranges["xaxis"]? axes["xaxis"]: (ranges["x2axis"]? axes["x2axis"]: axes["xaxis"]);
                range = ranges["xaxis"] || ranges["x2axis"] || { from:ranges["x1"], to:ranges["x2"] }
                selection.first.x = axis.p2c(Math.min(range.from, range.to));
                selection.second.x = axis.p2c(Math.max(range.from, range.to));
            }

            if (o.selection.mode == "x") {
                selection.first.y = 0;
                selection.second.y = plot.height();
            }
            else {
                axis = ranges["yaxis"]? axes["yaxis"]: (ranges["y2axis"]? axes["y2axis"]: axes["yaxis"]);
                range = ranges["yaxis"] || ranges["y2axis"] || { from:ranges["y1"], to:ranges["y2"] }
                selection.first.y = axis.p2c(Math.min(range.from, range.to));
                selection.second.y = axis.p2c(Math.max(range.from, range.to));
            }

            selection.show = true;
            plot.triggerRedrawOverlay();
            if (!preventEvent)
                triggerSelectedEvent();
        }

        function selectionIsSane() {
            var minSize = 5;
            return Math.abs(selection.second.x - selection.first.x) >= minSize &&
                Math.abs(selection.second.y - selection.first.y) >= minSize;
        }

        plot.clearSelection = clearSelection;
        plot.setSelection = setSelection;
        plot.getSelection = getSelection;

        plot.hooks.bindEvents.push(function(plot, eventHolder) {
            var o = plot.getOptions();
            if (o.selection.mode != null)
                eventHolder.mousemove(onMouseMove);

            if (o.selection.mode != null)
                eventHolder.mousedown(onMouseDown);
        });


        plot.hooks.drawOverlay.push(function (plot, ctx) {
            
            if (selection.show && selectionIsSane()) {
                var plotOffset = plot.getPlotOffset();
                var o = plot.getOptions();

                ctx.save();
                ctx.translate(plotOffset.left, plotOffset.top);

                var c = $.color.parse(o.selection.color);

                ctx.strokeStyle = c.scale('a', 0.8).toString();
                ctx.lineWidth = 1;
                ctx.lineJoin = "round";
                ctx.fillStyle = c.scale('a', 0.4).toString();

                var x = Math.min(selection.first.x, selection.second.x),
                    y = Math.min(selection.first.y, selection.second.y),
                    w = Math.abs(selection.second.x - selection.first.x),
                    h = Math.abs(selection.second.y - selection.first.y);

                ctx.fillRect(x, y, w, h);
                ctx.strokeRect(x, y, w, h);

                ctx.restore();
            }
        });
    }

    $.plot.plugins.push({
        init: init,
        options: {
            selection: {
                mode: null, 
                color: "#e8cfac"
            }
        },
        name: 'selection',
        version: '1.0'
    });
})(jQuery);
