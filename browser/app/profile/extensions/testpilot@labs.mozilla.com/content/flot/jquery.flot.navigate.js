






























































(function(E){E.fn.drag=function(L,K,J){if(K){this.bind("dragstart",L)}if(J){this.bind("dragend",J)}return !L?this.trigger("drag"):this.bind("drag",K?K:L)};var A=E.event,B=A.special,F=B.drag={not:":input",distance:0,which:1,dragging:false,setup:function(J){J=E.extend({distance:F.distance,which:F.which,not:F.not},J||{});J.distance=I(J.distance);A.add(this,"mousedown",H,J);if(this.attachEvent){this.attachEvent("ondragstart",D)}},teardown:function(){A.remove(this,"mousedown",H);if(this===F.dragging){F.dragging=F.proxy=false}G(this,true);if(this.detachEvent){this.detachEvent("ondragstart",D)}}};B.dragstart=B.dragend={setup:function(){},teardown:function(){}};function H(L){var K=this,J,M=L.data||{};if(M.elem){K=L.dragTarget=M.elem;L.dragProxy=F.proxy||K;L.cursorOffsetX=M.pageX-M.left;L.cursorOffsetY=M.pageY-M.top;L.offsetX=L.pageX-L.cursorOffsetX;L.offsetY=L.pageY-L.cursorOffsetY}else{if(F.dragging||(M.which>0&&L.which!=M.which)||E(L.target).is(M.not)){return }}switch(L.type){case"mousedown":E.extend(M,E(K).offset(),{elem:K,target:L.target,pageX:L.pageX,pageY:L.pageY});A.add(document,"mousemove mouseup",H,M);G(K,false);F.dragging=null;return false;case !F.dragging&&"mousemove":if(I(L.pageX-M.pageX)+I(L.pageY-M.pageY)<M.distance){break}L.target=M.target;J=C(L,"dragstart",K);if(J!==false){F.dragging=K;F.proxy=L.dragProxy=E(J||K)[0]}case"mousemove":if(F.dragging){J=C(L,"drag",K);if(B.drop){B.drop.allowed=(J!==false);B.drop.handler(L)}if(J!==false){break}L.type="mouseup"}case"mouseup":A.remove(document,"mousemove mouseup",H);if(F.dragging){if(B.drop){B.drop.handler(L)}C(L,"dragend",K)}G(K,true);F.dragging=F.proxy=M.elem=false;break}return true}function C(M,K,L){M.type=K;var J=E.event.handle.call(L,M);return J===false?false:J||M.result}function I(J){return Math.pow(J,2)}function D(){return(F.dragging===false)}function G(K,J){if(!K){return }K.unselectable=J?"off":"on";K.onselectstart=function(){return J};if(K.style){K.style.MozUserSelect=J?"":"none"}}})(jQuery);













(function(c){var a=["DOMMouseScroll","mousewheel"];c.event.special.mousewheel={setup:function(){if(this.addEventListener){for(var d=a.length;d;){this.addEventListener(a[--d],b,false)}}else{this.onmousewheel=b}},teardown:function(){if(this.removeEventListener){for(var d=a.length;d;){this.removeEventListener(a[--d],b,false)}}else{this.onmousewheel=null}}};c.fn.extend({mousewheel:function(d){return d?this.bind("mousewheel",d):this.trigger("mousewheel")},unmousewheel:function(d){return this.unbind("mousewheel",d)}});function b(f){var d=[].slice.call(arguments,1),g=0,e=true;f=c.event.fix(f||window.event);f.type="mousewheel";if(f.wheelDelta){g=f.wheelDelta/120}if(f.detail){g=-f.detail/3}d.unshift(f,g);return c.event.handle.apply(this,d)}})(jQuery);




(function ($) {
    var options = {
        xaxis: {
            zoomRange: null, 
            panRange: null 
        },
        zoom: {
            interactive: false,
            trigger: "dblclick", 
            amount: 1.5 
        },
        pan: {
            interactive: false
        }
    };

    function init(plot) {
        function bindEvents(plot, eventHolder) {
            var o = plot.getOptions();
            if (o.zoom.interactive) {
                function clickHandler(e, zoomOut) {
                    var c = plot.offset();
                    c.left = e.pageX - c.left;
                    c.top = e.pageY - c.top;
                    if (zoomOut)
                        plot.zoomOut({ center: c });
                    else
                        plot.zoom({ center: c });
                }
                
                eventHolder[o.zoom.trigger](clickHandler);

                eventHolder.mousewheel(function (e, delta) {
                    clickHandler(e, delta < 0);
                    return false;
                });
            }
            if (o.pan.interactive) {
                var prevCursor = 'default', pageX = 0, pageY = 0;
                
                eventHolder.bind("dragstart", { distance: 10 }, function (e) {
                    if (e.which != 1)  
                        return false;
                    eventHolderCursor = eventHolder.css('cursor');
                    eventHolder.css('cursor', 'move');
                    pageX = e.pageX;
                    pageY = e.pageY;
                });
                eventHolder.bind("drag", function (e) {
                    
                    
                });
                eventHolder.bind("dragend", function (e) {
                    eventHolder.css('cursor', prevCursor);
                    plot.pan({ left: pageX - e.pageX,
                               top: pageY - e.pageY });
                });
            }
        }

        plot.zoomOut = function (args) {
            if (!args)
                args = {};
            
            if (!args.amount)
                args.amount = plot.getOptions().zoom.amount

            args.amount = 1 / args.amount;
            plot.zoom(args);
        }
        
        plot.zoom = function (args) {
            if (!args)
                args = {};
            
            var axes = plot.getAxes(),
                options = plot.getOptions(),
                c = args.center,
                amount = args.amount ? args.amount : options.zoom.amount,
                w = plot.width(), h = plot.height();

            if (!c)
                c = { left: w / 2, top: h / 2 };
                
            var xf = c.left / w,
                x1 = c.left - xf * w / amount,
                x2 = c.left + (1 - xf) * w / amount,
                yf = c.top / h,
                y1 = c.top - yf * h / amount,
                y2 = c.top + (1 - yf) * h / amount;

            function scaleAxis(min, max, name) {
                var axis = axes[name],
                    axisOptions = options[name];
                
                if (!axis.used)
                    return;
                    
                min = axis.c2p(min);
                max = axis.c2p(max);
                if (max < min) { 
                    var tmp = min
                    min = max;
                    max = tmp;
                }

                var range = max - min, zr = axisOptions.zoomRange;
                if (zr &&
                    ((zr[0] != null && range < zr[0]) ||
                     (zr[1] != null && range > zr[1])))
                    return;
            
                axisOptions.min = min;
                axisOptions.max = max;
            }

            scaleAxis(x1, x2, 'xaxis');
            scaleAxis(x1, x2, 'x2axis');
            scaleAxis(y1, y2, 'yaxis');
            scaleAxis(y1, y2, 'y2axis');
            
            plot.setupGrid();
            plot.draw();
            
            if (!args.preventEvent)
                plot.getPlaceholder().trigger("plotzoom", [ plot ]);
        }

        plot.pan = function (args) {
            var l = +args.left, t = +args.top,
                axes = plot.getAxes(), options = plot.getOptions();

            if (isNaN(l))
                l = 0;
            if (isNaN(t))
                t = 0;

            function panAxis(delta, name) {
                var axis = axes[name],
                    axisOptions = options[name],
                    min, max;
                
                if (!axis.used)
                    return;

                min = axis.c2p(axis.p2c(axis.min) + delta),
                max = axis.c2p(axis.p2c(axis.max) + delta);

                var pr = axisOptions.panRange;
                if (pr) {
                    
                    if (pr[0] != null && pr[0] > min) {
                        delta = pr[0] - min;
                        min += delta;
                        max += delta;
                    }
                    
                    if (pr[1] != null && pr[1] < max) {
                        delta = pr[1] - max;
                        min += delta;
                        max += delta;
                    }
                }
                
                axisOptions.min = min;
                axisOptions.max = max;
            }

            panAxis(l, 'xaxis');
            panAxis(l, 'x2axis');
            panAxis(t, 'yaxis');
            panAxis(t, 'y2axis');
            
            plot.setupGrid();
            plot.draw();
            
            if (!args.preventEvent)
                plot.getPlaceholder().trigger("plotpan", [ plot ]);
        }
        
        plot.hooks.bindEvents.push(bindEvents);
    }
    
    $.plot.plugins.push({
        init: init,
        options: options,
        name: 'navigate',
        version: '1.1'
    });
})(jQuery);
