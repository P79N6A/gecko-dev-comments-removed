





















































(function ($) {
    var options = {
        crosshair: {
            mode: null, 
            color: "rgba(170, 0, 0, 0.80)",
            lineWidth: 1
        }
    };
    
    function init(plot) {
        
        var crosshair = { x: -1, y: -1, locked: false };

        plot.setCrosshair = function setCrosshair(pos) {
            if (!pos)
                crosshair.x = -1;
            else {
                var axes = plot.getAxes();
                
                crosshair.x = Math.max(0, Math.min(pos.x != null ? axes.xaxis.p2c(pos.x) : axes.x2axis.p2c(pos.x2), plot.width()));
                crosshair.y = Math.max(0, Math.min(pos.y != null ? axes.yaxis.p2c(pos.y) : axes.y2axis.p2c(pos.y2), plot.height()));
            }
            
            plot.triggerRedrawOverlay();
        };
        
        plot.clearCrosshair = plot.setCrosshair; 
        
        plot.lockCrosshair = function lockCrosshair(pos) {
            if (pos)
                plot.setCrosshair(pos);
            crosshair.locked = true;
        }

        plot.unlockCrosshair = function unlockCrosshair() {
            crosshair.locked = false;
        }

        plot.hooks.bindEvents.push(function (plot, eventHolder) {
            if (!plot.getOptions().crosshair.mode)
                return;

            eventHolder.mouseout(function () {
                if (crosshair.x != -1) {
                    crosshair.x = -1;
                    plot.triggerRedrawOverlay();
                }
            });
            
            eventHolder.mousemove(function (e) {
                if (plot.getSelection && plot.getSelection()) {
                    crosshair.x = -1; 
                    return;
                }
                
                if (crosshair.locked)
                    return;
                
                var offset = plot.offset();
                crosshair.x = Math.max(0, Math.min(e.pageX - offset.left, plot.width()));
                crosshair.y = Math.max(0, Math.min(e.pageY - offset.top, plot.height()));
                plot.triggerRedrawOverlay();
            });
        });

        plot.hooks.drawOverlay.push(function (plot, ctx) {
            var c = plot.getOptions().crosshair;
            if (!c.mode)
                return;

            var plotOffset = plot.getPlotOffset();
            
            ctx.save();
            ctx.translate(plotOffset.left, plotOffset.top);

            if (crosshair.x != -1) {
                ctx.strokeStyle = c.color;
                ctx.lineWidth = c.lineWidth;
                ctx.lineJoin = "round";

                ctx.beginPath();
                if (c.mode.indexOf("x") != -1) {
                    ctx.moveTo(crosshair.x, 0);
                    ctx.lineTo(crosshair.x, plot.height());
                }
                if (c.mode.indexOf("y") != -1) {
                    ctx.moveTo(0, crosshair.y);
                    ctx.lineTo(plot.width(), crosshair.y);
                }
                ctx.stroke();
            }
            ctx.restore();
        });
    }
    
    $.plot.plugins.push({
        init: init,
        options: options,
        name: 'crosshair',
        version: '1.0'
    });
})(jQuery);
