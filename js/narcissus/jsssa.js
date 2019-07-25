










































(function() {

    const parser = Narcissus.parser;
    const definitions = Narcissus.definitions;
    const tokens = definitions.tokens;
    const Node = parser.Node;
    const hasOwnProperty = Object.prototype.hasOwnProperty;

    
    eval(definitions.consts);

    const dash = new Node({ lineno: -1 }, INTERVENED);

    


    function GenHash(seed) {
        this.seed = seed || 1;
    }

    GenHash.prototype = {
        gen: function(name) {
            
            
            return name + "|" + this.seed++;
        }
    }

    const genhash = new GenHash();

    



    function Upvars(defs, uses, useTuples, intervenes, escapes, flowIUVs,
                    needsEscape, isEval) {
        this.__hash__ = genhash.gen("$upvars");
        this.defs = defs || new Map;
        this.uses = uses || new Map;
        this.useTuples = useTuples || new Map;
        this.intervenes = intervenes || new Map;
        this.escapes = escapes || new Map;

        
        this.flowIUVs = flowIUVs || new Map;

        this.needsEscape = needsEscape;
        this.isEval = isEval;
    }

    Upvars.prototype = {
        clone: function() {
            return new Upvars(this.defs.clone(),
                              this.uses.clone(),
                              this.useTuples.clone(),
                              this.intervenes.clone(),
                              this.escapes.clone(),
                              this.flowIUVs.clone(),
                              this.needsEscape,
                              this.isEval);
        },

        unionWith: function(r) {
            this.defs.unionWith(r.defs);
            this.uses.unionWith(r.uses);
            this.useTuples.unionWith(r.useTuples);
            this.intervenes.unionWith(r.intervenes);
            this.escapes.unionWith(r.escapes);
            this.flowIUVs.unionWith(r.flowIUVs);
            this.needsEscape = this.needsEscape || r.needsEscape;
            this.isEval = this.isEval || r.isEval;
        },

        
        
        transClosureI: function(visited) {
            var r = this.defs.clone();
            var members = this.intervenes.members;
            var uv;

            
            
            
            
            
            
            
            
            
            
            
            visited.insert1(this);

            var uvs;
            for (var i in members) {
                uv = members[i];

                
                
                if (uv.def === dash) {
                    return null;
                }

                uvs = uv.upvars;
                if (uvs && !visited.lookup(uvs)) {
                    var tci;
                    if (tci = uvs.transClosureI(visited)) {
                        r.unionWith(tci);
                    } else {
                        return null;
                    }
                }
            }

            return r;
        },

        
        
        transClosureE: function(visited) {
            var r = this.escapes.clone();
            var members = r.members;
            var uv;

            visited.insert1(this);

            
            
            
            var uvs;
            for (var i in members) {
                uv = members[i];

                
                
                if (uv.def === dash) {
                    return null;
                }

                uvs = uv.upvars;
                if (uvs && !visited.lookup(uvs)) {
                    var tci, tce;
                    if (tci = uvs.transClosureI(visited)) {
                        r.unionWith(tci);
                    } else {
                        return null;
                    }
                    if (tce = uvs.transClosureE(visited)) {
                        r.unionWith(tce);
                    } else {
                        return null;
                    }
                }
            }

            return r;
        }
    }

    
    
    
    function Current(name, type, def, upvars) {
        def = def || mkRawIdentifier({ lineno: -1 }, "undefined", null, true);

        this.__hash__ = genhash.gen(name);
        this.name = name;
        this.type = type;
        this._def = def;
        this.upvars = upvars;
        this.gotIntervened = false;
    }

    Current.prototype = {
        clone: function() {
            var c = new Current(this.name,
                                this.type,
                                this.def,
                                this.upvars.clone());
            c.gotIntervened = this.gotIntervened;
            return c;
        },

        get def() {
            return this.gotIntervened ? dash : this._def;
        },

        set def(d) {
            this._def = d;
        }
    };

    
    function Map() {
        this.members = {};
        this.length = 0;
    }

    function h(k) {
        return k.__hash__ ? k.__hash__ : k;
    }

    Map.prototype = {
        clone: function() {
            var c = new Map;
            c.unionWith(this);
            return c;
        },

        lookup: function(k) {
            return this.members[h(k)];
        },

        insert: function(k, v) {
            k = h(k);
            if (!hasOwnProperty.call(this.members, k)) {
                this.members[k] = v;
                this.length++;
            }
        },

        insert1: function(k) {
            this.insert(k, k);
        },

        remove: function(k) {
            k = h(k);
            if (hasOwnProperty.call(this.members, k)) {
                delete this.members[k];
                this.length--;
            }
        },

        unionWith: function(r) {
            var members = r.members;
            var keys = Object.getOwnPropertyNames(members);
            for (var i in keys) {
                var k = keys[i];
                this.insert(k, members[k]);
            }
        },

        clear: function() {
            this.members = {};
            this.length = 0;
        }
    }

    
    function Bindings(p, isFunction, isWith) {
        this.parent = p;
        this.isFunction = isFunction;
        this.isWith = isWith;
        this.currents = {};
        this.params = {};
        this.possibleHoists = {};
        this.dead = false;
        this.inRHS = 0;
        if (isWith) {
            this.withIntervenes = new Map;
        }
        if (isFunction) {
            
            this.frees = {};
            this.upvars = new Upvars;
            this.backpatchUpvars = new Map;
            
            
            this.escaped = new Upvars;
            
            this.upvarOlds = {};
            
            this.backpatchMus = [];
        }

        
        var r = this;
        while (!r.isFunction) {
            r = r.parent;
        }
        this.nearestFunction = r;
    }

    Bindings.prototype = {
        escapedVars: function() {
            if (this.evalEscaped) {
                return null;
            }

            return this.escaped.transClosureI(new Map);
        },

        declareParam: function(x) {
            this.params[x] = true;
        },

        declareVar: function(x, tt, isExternal) {
            if (this.dead) {
                return;
            }

            var fb = this.nearestFunction;
            var c = this.current(x);

            if (c) {
                if (c.type == LET) {
                    throw new TypeError("Cannot redeclare a let to a var");
                }
                if (c.type == CONST && tt == CONST) {
                    throw new TypeError("redeclaration of const " + x);
                }
            } else if (!hasOwnProperty.call(fb.params, x)) {
                
                
                
                
                
                
                
                
                var fb = this.nearestFunction;
                fb.currents[x] = c = new Current(x, tt);
                c.internal = !isExternal;

                return c.def;
            }
        },

        declareLet: function(x, hoisted, isExternal) {
            if (this.dead) {
                return;
            }

            var c = this.currents[x];
            if (c) {
                if (!c.hoisted)
                    throw new TypeError("Cannot redeclare a let");
                
                c.hoisted = false;
                return;
            }

            c = this.currents[x] = new Current(x, LET);
            c.hoisted = hoisted;
            c.internal = !isExternal;

            return c.def;
        },

        update: function(x, def, upvars) {
            if (this.dead) {
                return;
            }

            var c = this.current(x);
            if (!c) {
                
                c = this.upvar(x);
                var p = this.nearestFunction;
                if (!hasOwnProperty.call(p.upvarOlds, x)) {
                    p.upvarOlds[x] = { def: c.def,
                                       upvars: c.upvars.clone(),
                                       gotIntervened: c.gotIntervened };
                }
            }
            if (c.type == CONST) {
                return;
            }

            
            c.gotIntervened = false;
            c.def = def;
            c.upvars = upvars;
        },

        hasCurrent: function(x) {
            var p = this.isFunction ? null : this.parent;
            return hasOwnProperty.call(this.currents, x) ||
                   (p && p.hasCurrent(x));
        },

        hasParam: function(x) {
            var p = this.nearestFunction;
            return hasOwnProperty.call(p.params, x);
        },

        hasUpParam: function(x) {
            var p = this.nearestFunction;
            p = p.parent;
            if (p) {
                var r = p.hasParam(x);
                return r || p.hasUpParam(x);
            } else {
                return null;
            }
        },

        upvar: function(x) {
            
            
            
            
            
            
            
            var p = this.nearestFunction;
            p = p.parent;
            if (p) {
                var c = p.current(x);
                
                return c || p.upvar(x);
            } else {
                return null;
            }
        },

        addUpvar: function(tt, uv) {
            this.nearestFunction.upvars[tt].insert1(uv);
        },

        pushUpvarUse: function(uv, n) {
            var useTuples = this.nearestFunction.upvars.useTuples;
            if (!useTuples.lookup(uv)) {
                useTuples.insert(uv, { cdef: undefined, nodes: [] });
            }
            useTuples.lookup(uv).nodes.push(n);
        },

        pushMuUse: function(n) {
            this.nearestFunction.backpatchMus.push(n);
        },

        removeMuUse: function(n) {
            var nodes = this.nearestFunction.backpatchMus;
            for (var i in nodes) {
                if (n === nodes[i]) {
                    nodes[i] = nodes[nodes.length-1];
                    nodes.pop();
                    return;
                }
            }
        },

        removeUpvarUse: function(uv, n) {
            
            var useTuples = this.nearestFunction.upvars.useTuples;
            var nodes = useTuples.lookup(uv).nodes;
            for (var i in nodes) {
                if (n === nodes[i]) {
                    
                    
                    nodes[i] = nodes[nodes.length-1];
                    nodes.pop();
                    return;
                }
            }
        },

        addBackpatchUpvars: function(upvars) {
            this.nearestFunction.backpatchUpvars.insert1(upvars);
        },

        closureIsEval: function() {
            
            
            
            this.nearestFunction.isEval = true;
        },

        closureNeedsEscape: function() {
            
            
            
            
            this.nearestFunction.needsEscape = true;
        },

        addToFrees: function(x) {
            this.nearestFunction.frees[x] = true;
        },

        declareFunction: function(x, f, frees, upvars) {
            var fb = this.nearestFunction;
            if (!fb.currents[x]) {
                fb.currents[x] = new Current(f.name, VAR, f, upvars);
            }
        },

        mu: function(x) {
            
            
            var r = this;
            while (r) {
                var fb = r.nearestFunction;
                if (fb.name == x) {
                    return fb;
                } else {
                    r = r.parent;
                }
            }

            return null;
        },

        current: function(x) {
            var r = undefined;
            if (hasOwnProperty.call(this.currents, x)) {
                r = this.currents[x];
            } else if (!this.isFunction && this.parent) {
                
                
                r = this.parent.current(x);
            }

            return r;
        },

        unionUpTo: function(upTo) {
            var vars = new Map;
            var p = this;
            while (p) {
                var currents = p.currents;
                var cs = Object.getOwnPropertyNames(currents);
                var c;
                for (var i in cs) {
                    c = currents[cs[i]];
                    
                    if (c.type != CONST) {
                        vars.insert1(c);
                    }
                }
                if (p === upTo)
                    return vars;
                else
                    p = p.parent;
            }

            
            return vars;
        },

        intervene: function(vars) {
            
            
            if (!vars) {
                vars = this.unionUpTo(null);
            }

            var uv, intervened = [];
            var members = vars.members;
            for (var i in members) {
                uv = members[i];
                
                
                if (!this.hasCurrent(uv.name)) {
                    var p = this.nearestFunction;
                    var x = uv.name;

                    if (uv.upvars) {
                        
                        var fiuvs = uv.upvars.flowIUVs.members;
                        for (var j in fiuvs) {
                            this.addUpvar("escapes", fiuvs[j]);
                        }
                    }

                    if (!hasOwnProperty.call(p.upvarOlds, x)) {
                        p.upvarOlds[x] = { def: uv.def,
                                           upvars: uv.upvars,
                                           gotIntervened: uv.gotIntervened };
                    }
                }
                
                
                intervened.push({ ptr: uv,
                                  def: uv.def,
                                  upvars: uv.upvars, });
                uv.def = undefined;
                uv.upvars = undefined;
                
                
                uv.gotIntervened = true;
            }

            return intervened;
        },

        rememberPossibleHoist: function(x) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            this.possibleHoists[x] = true;
        },

        isPossibleHoist: function(x) {
            return hasOwnProperty.call(this.possibleHoists, x) &&
                   !hasOwnProperty.call(this.params, x);
        },

        propagatePossibleHoists: function() {
            var p = this.inFunction ? this.parent.nearestFunction
                                    : this.parent;
            if (!p)
                return;

            var hoists = Object.getOwnPropertyNames(this.possibleHoists);
            var x;
            for (var i in hoists) {
                x = hoists[i];
                if (!hasOwnProperty.call(p.currents, x)) {
                    p.rememberPossibleHoist(x);
                }
            }
        },
    };

    function SSAJoin(p, b, before) {
        this.parent = p;
        this.binds = b;
        this.hasJoinBefore = before;
        this.branch = 0;
        this.phis = {};
        this.uses = {};

        
        
    }

    SSAJoin.mkJoin = function(ps, parent, branches, propagate) {
        
        if (branches < 1) {
            return null;
        }

        var e, e2, rhs;

        function phiAssignment(x, operands) {
            var e, lhs, rhs;
            var ft = { lineno: -1 };

            lhs = new Node(ft, IDENTIFIER);
            lhs.value = x;
            
            lhs.local = operands.type;
            rhs = new Node(ft, PHI);
            rhs.__hash__ = genhash.gen("$phi");

            
            
            var allDashes = true;
            var os, o = null;
            for (var i = branches - 1; i >= 0; i--) {
                o = operands[i] ? operands[i].def : operands.old.def;
                if (o.type != INTERVENED)
                    allDashes = false;
                rhs.push(o);
            }
            if (allDashes) {
                return null;
            }
            rhs.reverse();

            e = new Node(ft, ASSIGN);
            e.push(lhs);
            e.push(rhs);

            return e;
        }

        e = new Node({ lineno: -1 }, COMMA);
        for (var x in ps) {
            if (!(e2 = phiAssignment(x, ps[x]))) {
                continue;
            }

            rhs = e2[1];

            
            
            if (branches == 1) {
                rhs = rhs[0];
            } else {
                
                
                for (var i = 0, j = rhs.length; i < j; i++) {
                    if (rhs[i].type == INTERVENED) {
                        rhs.intervened = true;
                    }
                    rhs[i].pushPhiUse(rhs);
                }
                e.push(e2);
            }

            propagate(x, rhs);
        }

        return e.length > 0 ? e : null;
    }

    SSAJoin.prototype = {
        nearestTryJoin: function() {
            if (this.isTry) {
                return this;
            } else if (this.parent) {
                return this.parent.nearestTryJoin();
            } else {
                return null;
            }
        },

        unionPhisUpTo: function(joinUpTo) {
            var ps = {}, join = this, binds = join.binds;
            var bindsUpTo = joinUpTo.binds;
            while (join) {
                for (var x in join.phis) {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    if (bindsUpTo.current(x) === binds.current(x)) {
                        ps[x] = join.phis[x];
                    }
                }
                if (join === joinUpTo) {
                    break;
                }
                join = join.parent;
                binds = join.binds;
            }
            return ps;
        },

        shiftPhis: function() {
            var ps = this.phis;
            for (var x in ps) {
                ps[x].shift();
            }
            this.branch--;
        },

        killBranch: function(currentBinds, killJoin, isThrow) {
            
            currentBinds.dead = true;

            
            
            var tryJoin = this.nearestTryJoin();
            if (tryJoin && tryJoin&& !isThrow) {
                var oldKillJoin = killJoin;
                killJoin = tryJoin.isFinally ? tryJoin : tryJoin.parent;
            }

            if (!killJoin) {
                this.dead = true;
                return;
            }

            var killBinds = killJoin.binds;
            
            
            var ps = this.unionPhisUpTo(killJoin);

            
            var c, old;
            for (var x in ps) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                c = killBinds.current(x) || killBinds.upvar(x);
                old = ps[x].old;
                killJoin.insertPhi(c.type, x, c.def, c.upvars,
                                   old.def, old.upvars);
            }

            killJoin.finishBranch();
            killJoin.upkill = oldKillJoin;

            
            
            this.dead = true;
        },

        finishBranch: function() {
            if (!this.dead) {
                this.branch++;
            }
        },

        commit: function() {
            
            var r, e;
            var ps = this.phis, us = this.uses
            var parent = this.parent;
            var binds = this.binds;
            var ft = { lineno: -1 };
            var intervened = false;

            function go(x, rhs) {
                var u = us[x], uu;
                var old = ps[x].old;
                var prop = ps[x].prop;

                if (u) {
                    for (var i = 0, j = u.length; i < j; i++) {
                        
                        
                        
                        
                        
                        
                        
                        
                        uu = u[i];
                        
                        if (uu.type == PHI) {
                            for (var k = 0, l = uu.length; k < l; k++) {
                                if (uu[k] === old.def) {
                                    uu[k] = rhs;
                                    rhs.pushPhiUse(uu);
                                }
                            }
                        } else if (uu.forward === old.def) {
                            uu.forward = rhs;
                        }
                    }
                }

                var upvars = unionPhiUpvars(ps[x]);

                
                if (prop) {
                    rhs = prop.def;
                    upvars = prop.upvars;
                }

                
                var type = ps[x].type;
                if (parent && (type == VAR || !binds.currents[x])) {
                    parent.insertPhi(ps[x].type, x, rhs, upvars,
                                     old.def, old.upvars);
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    parent.rememberUse(x, rhs);
                }

                binds.update(x, rhs, upvars);
            }

            e = SSAJoin.mkJoin(ps, parent, this.branch, go);
            if (!e) {
                
                
                if (this.branch < 1) {
                    this.restore(binds);
                }
                return null;
            }

            r = new Node(ft, SEMICOLON);
            r.expression = e;
            return r;
        },

        
        rememberUse: function(x, v) {
            if (this.hasJoinBefore) {
                var uses = this.uses;
                if (!hasOwnProperty.call(uses, x)) {
                    uses[x] = [];
                }
                uses[x].push(v);
            } else {
                
                this.parent && this.parent.rememberUse(x, v);
            }
        },

        insertDashes: function(binds, intervened) {
            var iv, uv, c, name;
            for (var x in intervened) {
                iv = intervened[x];
                uv = iv.ptr;
                name = uv.name;
                c = binds.current(name);
                
                
                if (uv === c && !c.internal &&
                    (uv.type == VAR || !binds.currents[name])) {
                    this.insertPhi(c.type, name, dash, new Upvars,
                                   iv.def, iv.upvars);
                }
            }
        },

        intervene: function(binds, vars) {
            var intervened = this.intervened = binds.intervene(vars);
            this.insertDashes(binds, intervened);
            return intervened;
        },

        
        restore: function(binds) {
            binds.dead = false;
            this.dead = false;

            var intervened = this.intervened;
            if (intervened) {
                var iv;
                for (var x in intervened) {
                    iv = intervened[x];
                    iv.ptr.def = iv.def;
                    iv.ptr.upvars = iv.upvars;
                }
            }

            var ps = this.phis;
            var old;
            for (var x in ps) {
                old = ps[x].old;
                binds.update(x, old.def, old.upvars);
            }
        },

        
        
        insertPhi: function(type, x, def, upvars,
                            oldDef, oldUpvars,
                            propDef, propUpvars) {
            var n, ps = this.phis;
            var psx;

            if (!ps || this.dead) {
                return;
            }
            if (!ps[x]) {
                psx = ps[x] = [];
                psx.use = [];
            } else {
                psx = ps[x];
            }

            psx[this.branch] = { def: def, upvars: upvars };
            
            if (propDef) {
                psx.prop = { def: propDef, upvars: propUpvars };
            }
            if (!psx.old) {
                psx.old = { def: oldDef, upvars: oldUpvars };
            }
            if (!psx.type) {
                psx.type = type;
            }
        }
    };

    



    function extendBuilder(child, super) {
        var childProto = child.prototype,
            superProto = super.prototype;
        for (var ns in super.prototype) {
            var childNS = childProto[ns];
            var superNS = superProto[ns];
            var childNSType = typeof childNS;
            if (childNSType === "undefined") {
                childProto[ns] = superNS;
            } else if (childNSType === "object") {
                for (var m in superNS) {
                    let childMethod = childNS[m];
                    let superMethod = superNS[m];
                    if (typeof childMethod === "undefined") {
                        childNS[m] = superMethod;
                    } else {
                        childNS[m] = function() {
                            if (this.binds)
                                return childMethod.apply(this, arguments);
                            else
                                return superMethod.apply(this, arguments);
                        };
                    }
                }
            }
        }
    }

    function SSABuilder() {
        parser.bindSubBuilders(this, SSABuilder.prototype);

        this.binds = null;
        this.join = null;
        this.hoists = {};
        this.inJoinPostDom = [false];
        this.finallyKills = [];
        this.joinStack = [];

        this.destructuringTmpFresh = 0;
        this.postfixTmpFresh = 0;
    }

    
    
    

    SSABuilder.prototype = {
        IF: {
            setCondition: function(n, e) {
                n.condition = e;
                
                
                this.join = new SSAJoin(this.join, this.binds, false);
            },

            setThenPart: function(n, s) {
                var join = this.join;
                n.thenPart = s;
                join.finishBranch();
                join.restore(this.binds);
            },

            finish: function(n) {
                var join = this.join;
                this.join = join.parent;
                join.finishBranch();
                n.ssaJoin = join.commit();
            }
        },

        SWITCH: {
            setDiscriminant: function(n, e) {
                var join = this.join = new SSAJoin(this.join, this.binds, false);
                n.discriminant = e;
                n.breakJoin = join;
                n.continueJoin = join;
                this.fallthrough = false;
            },

            addCase: function(n, n2) {
                var join = this.join;
                if (join.dead) {
                    
                    
                    
                    
                    
                    join.restore(this.binds);
                    this.fallthrough = false;
                } else {
                    
                    this.fallthrough = true;
                }
                n.cases.push(n2);
            },

            finish: function(n) {
                var join = this.join;
                this.join = join.parent;
                if (this.fallthrough)
                    join.finishBranch();
                n.ssaJoin = join.commit();
            }
        },

        CASE: {
            initializeStatements: function(n, t) {
                n.statements = new Node(t, BLOCK);
                
                if (this.fallthrough) {
                    n.ssaJoin = fallthroughJoin(n, this.join, this.binds);
                }
            }
        },

        DEFAULT: {
            initializeStatements: function(n, t) {
                n.statements = new Node(t, BLOCK);
                
                if (this.fallthrough) {
                    n.ssaJoin = fallthroughJoin(n, this.join, this.binds);
                }
            }
        },

        FOR: {
            build: function(t) {
                var n = new Node(t, FOR);
                
                
                
                
                
                
                
                
                
                
                
                
                
                var breakJoin = new SSAJoin(this.join, this.binds, false);
                var continueJoin = new SSAJoin(breakJoin, this.binds, true);
                n.isLoop = true;
                n.breakJoin = breakJoin;
                n.continueJoin = continueJoin;
                continueJoin.finishBranch();
                breakJoin.finishBranch();
                this.join = breakJoin;

                return n;
            },

            setObject: function(n, e) {
                var t = n.tokenizer;
                var itrhs = mkCall("iterator", e);
                
                n.setup = mkDecl(this, "LET", t, "$it", itrhs, false);
                n.condition = mkCall("hasNext", mkIdentifier(this, t, "$it"));
            },

            setSetup: function(n, e) {
                n.setup = e || null;
                this.join = n.continueJoin;
                this.inJoinPostDom.push(true);
            },

            setCondition: function(n, e) {
                n.condition = e;
                this.inJoinPostDom.pop();
            },

            setIterator: function(n, e, e2, s) {
                var setup = n.setup;

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                var dexp = e.exp;
                var decl = e.decl;
                var t = dexp ? dexp.tokenizer : e.tokenizer;
                var rhs = mkCall("next", mkIdentifier(this, t, "$it"));
                if (e2) {
                    if (dexp) {
                        if (e2.type == LET) {
                            
                            
                            
                            
                            var bComma = this.COMMA;

                            e2.push(n.setup[0]);
                            n.setup = e2;

                            var comma = bComma.build(t);
                            desugarDestructuringAssign(this, comma, dexp, rhs);
                            bComma.finish(comma);
                            n.update = comma;
                        } else  {
                            var bVar = this.VAR;
                            var bDecl = this.DECL;
                            bDecl.setInitializer(decl, rhs);
                            bDecl.finish(decl);
                            bVar.finish(e2);
                            n.update = e2;
                        }
                    } else if (e2.type == VAR) {
                        var bDecl = this.DECL;
                        bDecl.setInitializer(e, rhs);
                        n.update = e2;
                    } else if (e2.type == LET) {
                        n.setup.unshift(e);
                        n.update = mkAssign(this, t, e, rhs);
                    }
                } else {
                    n.update = mkAssign(this, t, e, rhs);
                }
                this.join = n.continueJoin;
            },

            setBody: function(n, s) {
                n.body = s;
                this.join.finishBranch();
            },

            finish: function(n) {
                var continueJoin = this.join, breakJoin = continueJoin.parent;
                this.join = breakJoin.parent;
                
                if (n.type == FOR_IN) {
                    n.body.unshift(n.update);
                    n.update = null;
                    n.type = FOR;
                }
                n.ssaJoin = continueJoin.commit();
                if (n.ssaJoin) {
                    
                    
                    
                    
                    breakJoin.shiftPhis();
                    breakJoin.finishBranch();
                }
                n.ssaBreakJoin = breakJoin.commit();
            }
        },

        WHILE: {
            build: function(t) {
                var n = new Node(t, WHILE);
                var breakJoin = new SSAJoin(this.join, this.binds, false);
                var continueJoin = new SSAJoin(breakJoin, this.binds, true);
                n.isLoop = true;
                n.breakJoin = breakJoin;
                n.continueJoin = continueJoin;
                
                
                continueJoin.finishBranch();
                breakJoin.finishBranch();
                this.join = continueJoin;
                
                
                this.inJoinPostDom.push(true);
                return n;
            },

            setCondition: function(n, e) {
                n.condition = e;
                this.inJoinPostDom.pop();
            },

            setBody: function(n, s) {
                n.body = s;
                this.join.finishBranch();
            },

            finish: function(n) {
                var continueJoin = this.join, breakJoin = continueJoin.parent;
                this.join = breakJoin.parent;
                n.ssaJoin = continueJoin.commit();
                if (n.ssaJoin) {
                    
                    
                    breakJoin.shiftPhis();
                    breakJoin.finishBranch();
                }
                n.ssaBreakJoin = breakJoin.commit();
            }
        },

        DO: {
            build: function(t) {
                var n = new Node(t, DO);
                var breakJoin = new SSAJoin(this.join, this.binds, true);
                var continueJoin = new SSAJoin(breakJoin, this.binds, true);
                n.isLoop = true;
                n.breakJoin = breakJoin;
                n.continueJoin = continueJoin;
                
                
                continueJoin.finishBranch();
                breakJoin.finishBranch();
                this.join = continueJoin;
                
                this.inJoinPostDom.push(true);
                return n;
            },

            setBody: function(n, s) {
                n.body = s;
                this.join.finishBranch();
            },

            setCondition: function(n, e) {
                n.condition = e;
                this.inJoinPostDom.pop();
            },

            finish: function(n) {
                var continueJoin = this.join, breakJoin = continueJoin.parent;
                this.join = breakJoin.parent;
                n.ssaJoin = continueJoin.commit();
                if (n.ssaJoin) {
                    
                    
                    
                    
                    breakJoin.shiftPhis();
                    breakJoin.finishBranch();
                }
                
                
                
                
                
                
                
                
                n.ssaBreakJoin = breakJoin.commit();
            }
        },

        BREAK: {
            finish: function(n) {
                
                
                
                
                
                
                this.join.killBranch(this.binds, n.target.breakJoin);
            }
        },

        CONTINUE: {
            finish: function(n) {
                
                
                
                
                
                
                this.join.killBranch(this.binds, n.target.continueJoin);
            }
        },

        TRY: {
            build: function(t) {
                var n = new Node(t, TRY);
                var finallyJoin = new SSAJoin(this.join, this.binds, false);
                var tryJoin = new SSAJoin(finallyJoin, this.binds, false);
                tryJoin.isTry = true;
                n.catchClauses = [];
                this.join = tryJoin;
                return n;
            },

            setTryBlock: function(n, s) {
                var tryJoin = this.join, finallyJoin = tryJoin.parent;
                var binds = this.binds;
                this.join = finallyJoin;
                
                
                finallyJoin.isTry = finallyJoin.isFinally = true;
                n.tryBlock = s;
                
                
                
                if (!tryJoin.dead) {
                    var c, old;
                    for (var x in tryJoin.phis) {
                        c = binds.current(x) || binds.upvar(x);
                        old = tryJoin.phis[x].old;
                        finallyJoin.insertPhi(c.type, x, c.def, c.upvars,
                                              old.def, old.upvars);
                    }
                    finallyJoin.finishBranch();
                }
                this.tryPhis = tryJoin.commit();
                
                
                
                for (var x in tryJoin.phis) {
                    var c = binds.current(x) || binds.upvar(x);
                    finallyJoin.phis[x].old = { def: c.def, upvars: c.upvars };
                }
            },

            finishCatches: function(n) {
                var finallyJoin = this.join;
                this.join = finallyJoin.parent;
                
                n.ssaFinallyJoin = finallyJoin.commit();
                this.finallyKills.push(finallyJoin.upkill);
            },

            finish: function(n) {
                
                var upkill = this.finallyKills.pop();
                if (upkill && !this.join.dead) {
                    var killBinds = upkill.binds;
                    var ps = this.join.unionPhisUpTo(upkill);
                    for (var x in ps) {
                        c = killBinds.current(x), old = ps[x].old;
                        upkill.insertPhi(c.type, x, c.def, c.upvars,
                                         old.def, old.upvars);
                    }
                    upkill.finishBranch();
                    this.join.dead = true;
                }

                if (!n.finallyBlock) {
                    
                    
                    n.ssaJoin = n.ssaFinallyJoin;
                    n.ssaFinallyJoin = undefined;
                }
            }
        },

        CATCH: {
            build: function(t) {
                
                
                this.binds = new Bindings(this.binds, false, false);
                this.noBindingsOnNextBlock = true;
                var n = new Node(t, CATCH);
                n.guard = null;
                return n;
            },

            setVarName: function(n, v) {
                var binds = this.binds;

                if (v.type == ARRAY_INIT || v.type == OBJECT_INIT) {
                    
                    var t = n.tokenizer;
                    var bLet = this.LET;
                    var bDecl = this.DECL;

                    var name = this.genDestructuringSym();
                    var lets = bLet.build(t);

                    var decl = bDecl.build(t);
                    bDecl.setName(decl, v);
                    bLet.addDestructuringDecl(lets, decl);
                    var rhs = mkRawIdentifier(t, name, null, false);
                    decl.initializer = rhs;
                    bDecl.finish(decl);
                    bLet.finish(lets);

                    v = name;
                    n.destructuredLets = lets;
                }

                binds.declareLet(v);
                
                
                
                binds.update(v, null);
                binds.current(v).catchLet = true;

                n.varName = v;
            },

            finish: function(n) {
                var p;
                var join = this.join;

                if (n.destructuredLets) {
                    n.block.unshift(n.destructuredLets);
                }

                n.ssaJoin = this.tryPhis;
                join.restore(this.binds);
                
                if (join.maybeThrows) {
                    join.finishBranch();
                }
            }
        },

        THROW: {
            finish: function(n) {
                
                
                
                var join = this.join, tryJoin = join && join.nearestTryJoin();
                join && join.killBranch(this.binds, tryJoin, true);
                if (tryJoin && tryJoin.parent) {
                    tryJoin.parent.maybeThrows = true;
                }
            }
        },

        RETURN: {
            finish: function(n) {
                
                
                var join = this.join;
                join && join.killBranch(this.binds, null);
            }
        },

        YIELD: {
            build: function(t) {
                this.binds.inRHS++;
                return new Node(t, YIELD);
            },

            setValue: function(n, e) {
                
                var join = this.join;
                var binds = this.binds;

                escapeVars(join, binds, e.upvars || new Upvars);
                n.value = e;
                --binds.inRHS;
            },

            finish: function(n) {
                
                
                
                var join = this.join;
                var binds = this.binds;
                var fb = binds.nearestFunction;
                var escaped = fb.escapedVars();

                binds.closureNeedsEscape();

                if (join) {
                    join.intervene(binds, escaped);
                } else {
                    binds.intervene(escaped);
                }
            }
        },

        FUNCTION: {
            setName: function(n, v) {
                n.name = v;
                
                
                
                
                
                
                this.binds.name = v;
            },

            addParam: function(n, v) {
                n.params.push(v);
                this.binds.declareParam(v);
            },

            hoistVars: function(id, toplevel) {
                var binds = this.binds;
                var vds = this.hoists[id];

                if (toplevel) {
                    
                    binds = this.binds = new Bindings(binds.parent, true, false);
                    binds.noPop = true;
                    this.noBindingsOnNextBlock = true;
                }
                if (!vds)
                    return;

                var name, init;

                var vd;
                for (var i = 0, j = vds.length; i < j; i++) {
                    vd = vds[i];
                    name = vd.name;
                    if (vd.type == FUNCTION) {
                        
                        
                        
                        
                        
                        
                        var frees = vd.frees;
                        var upvars = vd.upvars = new Upvars;
                        var c;

                        for (var x in frees) {
                            c = binds.current(x) || binds.upvar(x);
                            if (c) {
                                upvars.defs.insert1(c);
                                delete frees[x];
                            }
                        }

                        binds.declareFunction(name, vd, frees, upvars);
                    } else {
                        binds.declareVar(name, vd.readOnly ? CONST : VAR);
                    }
                }

                this.hoists[id] = undefined;
            },

            finish: function(n, x) {
                var binds = this.binds;
                this.binds.propagatePossibleHoists();
                var p = this.binds = binds.parent;

                n.frees = binds.frees;
                n.upvars = binds.upvars;
                n.upvars.needsEscape = binds.needsEscape;
                n.upvars.isEval = binds.isEval;

                this.join = this.joinStack.pop();

                if (p) {
                    var name = n.name;
                    if (name) {
                        var c = p.current(name);
                        var ff = n.functionForm;
                        var pfb = p.nearestFunction;

                        if (ff == parser.DECLARED_FORM) {
                            if (pfb.isPossibleHoist(name)) {
                                x.needsHoisting = true;
                            }

                            
                            
                            
                            if (!c) {
                                pfb.declareFunction(name, n, n.frees, n.upvars);
                            }
                        } else if (n.functionForm == parser.STATEMENT_FORM) {
                            
                            
                            
                            pfb.declareVar(name, VAR);
                            mkAssignSimple(this, n.tokenizer, name, n, true);
                        }
                    }

                    var uvs = Object.getOwnPropertyNames(binds.upvarOlds);
                    var old, c, x;
                    var upvarOlds = binds.upvarOlds;
                    var p2;
                    for (var i in uvs) {
                        x = uvs[i];
                        old = upvarOlds[x];

                        
                        
                        
                        
                        
                        p2 = p;
                        while (p2 && !p2.hasCurrent(x)) {
                            p2 = p2.parent;
                        }

                        if (p2) {
                            p2.update(x, old.def, old.upvars);
                            c = p2.current(x);
                            c.gotIntervened = old.gotIntervened;
                        }
                    }
                }

                
                var mus = binds.backpatchMus;
                for (var i in mus) {
                    mus[i].setForward(n);
                }

                
                
                var backpatches = binds.backpatchUpvars.members;
                for (var i in backpatches) {
                    var bp = backpatches[i];
                    var uvuses = bp.uses.members;
                    for (var j in uvuses) {
                        var use = uvuses[j];
                        var useTuple = bp.useTuples.lookup(use);
                        var cdef = useTuple.cdef;

                        if (cdef === undefined)
                            continue;

                        var unodes = useTuple.nodes;
                        for (var k in unodes) {
                            
                            unodes[k].setForward(cdef, true);
                        }
                    }
                }
            }
        },

        VAR: {
            addDestructuringDecl: mkAddDestructuringDecl(VAR, false),

            addDecl: function(n, n2, x) {
                var name = n2.name;
                var binds = this.binds;

                n2.initializer = binds.declareVar(name, VAR, !n2.internal);

                if (binds.nearestFunction.isPossibleHoist(name)) {
                    x.needsHoisting = true;
                }

                n.push(n2);
                x && x.varDecls.push(mkHoistDecl(this, n2));
            }
        },

        CONST: {
            addDestructuringDecl: mkAddDestructuringDecl(CONST, true),

            addDecl: function(n, n2, x) {
                var name = n2.name;
                var binds = this.binds;

                n2.initializer = binds.declareVar(name, CONST, !n2.internal);

                if (binds.nearestFunction.isPossibleHoist(name)) {
                    x.needsHoisting = true;
                }

                n.push(n2);
                x && x.varDecls.push(mkHoistDecl(this, n2));
            }
        },

        LET: {
            addDestructuringDecl: mkAddDestructuringDecl(LET, false),

            addDecl: function(n, n2, s) {
                var name = n2.name;
                var binds = this.binds;

                n2.initializer = binds.declareLet(name, false, !n2.internal);

                if (binds.isPossibleHoist(name)) {
                    s.needsHoisting = true;
                }

                n.push(n2);
                s && s.varDecls.push(mkHoistDecl(this, n2));
            }
        },

        DECL: {
            build: function(t) {
                this.binds.inRHS++;
                return new Node(t, IDENTIFIER);
            },

            setInitializer: function(n, e) {
                if (!n.destructuredDecls) {
                    var name = n.name;
                    var c = this.binds.current(name);

                    
                    
                    
                    
                    if (c) {
                        if (c.type == CONST) {
                            
                            
                            c.type = VAR;
                            mkAssignSimple(this, e.tokenizer,
                                           n.name, e, !n.internal);
                            c.type = CONST;
                        } else {
                            mkAssignSimple(this, e.tokenizer,
                                           n.name, e, !n.internal);
                        }
                    }

                    n.initializer = e;
                    return;
                }
                desugarDestructuringInit(this, n, e);
            },

            finish: function(n) {
                --this.binds.inRHS;
            }
        },

        LET_BLOCK: {
            build: function(t) {
                this.binds = new Bindings(this.binds, false, false);
                this.binds.noPop = true;
                this.noBindingsOnNextBlock = true;
                var n = new Node(t, LET_BLOCK);
                n.varDecls = [];
                return n;
            },

            finish: function(n) {
                this.binds.propagatePossibleHoists();
                this.binds = this.binds.parent;
            }
        },

        BLOCK: {
            build: function(t, id) {
                
                
                
                
                
                
                
                
                
                
                var n = new Node(t, BLOCK);
                n.varDecls = [];
                n.id = id;
                if (this.noBindingsOnNextBlock) {
                    this.binds.block = n;
                    this.binds.id = id;
                    this.noBindingsOnNextBlock = false;
                    return n;
                }
                this.binds = new Bindings(this.binds, false, false);
                this.binds.block = n;
                this.binds.id = id;
                return n;
            },

            hoistLets: function(n) {
                var lds = this.hoists[n.id];
                if (!lds)
                    return;

                var binds = this.binds;
                var name, init;

                binds = this.binds = new Bindings(binds, false, false);
                n.seenLet = true;

                for (var i = 0, j = lds.length; i < j; i++) {
                    name = lds[i].name;
                    binds.declareLet(name, true);
                }

                this.hoists[n.id] = undefined;
            },

            finish: function(n) {
                if (this.binds.noPop)
                    return;

                this.binds.propagatePossibleHoists();
                this.binds = this.binds.parent;
            }
        },

        ASSIGN: {
            addOperand: function(n, n2) {
                if (n.length == 0) {
                    this.binds.inRHS++;
                }

                n.push(n2);
            },

            finish: function(n) {
                if (n.length == 0) {
                    return;
                }

                var join = this.join;
                var binds = this.binds;
                var fb = binds.nearestFunction;
                var lhs = n[0];
                var init = n[1];
                var upvars = init.upvars || new Upvars;

                if (--binds.inRHS > 0) {
                    n.upvars = init.upvars;
                }

                
                if (lhs.type == ARRAY_INIT || lhs.type == OBJECT_INIT) {
                    var t = n.tokenizer;
                    
                    n.type = COMMA;
                    n.length = 0;
                    desugarDestructuringAssign(this, n, lhs, init);
                    return;
                }

                if (lhs.type != IDENTIFIER) {
                    escapeVars(join, binds, upvars);
                    return;
                }

                
                
                var name = lhs.value;
                var c = binds.current(name);
                var uv = binds.upvar(name);
                var hp = binds.hasParam(name);
                var hup = binds.hasUpParam(name);
                var mb = fb.mu(name);

                
                if (mb && (!c && !uv && !hup && !hp)) {
                    binds.removeMuUse(lhs);
                }

                
                if (c && binds.isWith && c.type == VAR) {
                    binds.withIntervenes.insert1(c);
                    c = null;
                    uv = null;
                }

                if (uv && !c) {
                    
                    
                    binds.addToFrees(name);
                    binds.addUpvar("defs", uv);

                    
                    
                    
                    var suv = new Upvars;
                    suv.defs.insert1(uv);
                    escapeVars(join, binds, suv);

                    c = uv;
                }

                if (c) {
                    if (n.assignOp) {
                        
                        
                        var nt = n.tokenizer;
                        var lhs = n[0];
                        var n2 = mkRawIdentifier(nt, name, null, true);
                        this.PRIMARY.finish(n2);
                        var o = n.assignOp;
                        n.assignOp = undefined;
                        n.length = 0;
                        n.push(lhs);
                        n.push(new Node(nt, o, n2, init));
                        n2.setForward(c.def);
                        return this.ASSIGN.finish(n);
                    }

                    
                    if (n[0].forward) {
                        n[0].forward = null;
                        n[0].upvars = null;
                    }
                    
                    n[0].local = c.type;

                    
                    
                    while (init.type == ASSIGN)
                        init = init[1];

                    if (join) {
                        
                        if (c.type == VAR) {
                            var propInit = null;
                            var propUpvars = null;
                            if (this.inJoinPostDom.top()) {
                                propInit = init;
                                propUpvars = upvars.clone();
                            }
                            join.insertPhi(VAR, name, init,
                                           upvars.clone(),
                                           c.def, c.upvars,
                                           propInit, propUpvars);
                        } else if (!binds.currents[name]) {
                            join.insertPhi(LET, name, init,
                                           upvars.clone(),
                                           c.def, c.upvars);
                        }
                    }

                    
                    
                    binds.update(name, init, upvars.clone());
                } else {
                    binds.addToFrees(name);

                    
                    
                    
                    escapeVars(join, binds, upvars);
                }
            }
        },

        HOOK: {
            



            build: function(t) {
                var n = new Node(t, HOOK);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            setCondition: function(n, e) {
                n[0] = e;
                n.rhsUnionUpvars(e);
                this.join = new SSAJoin(this.join, this.binds, false);
            },

            setThenPart: function(n, n2) {
                var join = this.join;
                n[1] = n2;
                n.rhsUnionUpvars(n2);
                join.finishBranch();
                join.restore(this.binds);
            },

            setElsePart: function(n, n2) {
                n[2] = n2;
                n.rhsUnionUpvars(n2);
            },

            finish: function(n) {
                var join = this.join;
                this.join = join.parent;
                join.finishBranch();
                n.ssaJoin = join.commit();
            }
        },

        OR: {
            build: function(t) {
                var n = new Node(t, OR);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            addOperand: function(n, n2) {
                if (n.length == 0) {
                    
                    
                    var join = this.join = new SSAJoin(this.join, this.binds, false);
                    
                    
                    join.finishBranch();
                }
                n.rhsUnionUpvars(n2);
                n.push(n2);
            },

            finish: function(n) {
                var join = this.join;
                this.join = join.parent;
                join.finishBranch();
                n.ssaJoin = join.commit();
            }
        },

        AND: {
            build: function(t) {
                var n = new Node(t, AND);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            addOperand: function(n, n2) {
                if (n.length == 0) {
                    
                    
                    var join = this.join = new SSAJoin(this.join, this.binds, false);
                    
                    
                    join.finishBranch();
                }
                n.push(n2);
            },

            finish: function(n) {
                var join = this.join;
                this.join = join.parent;
                join.finishBranch();
                n.ssaJoin = join.commit();
            }
        },

        UNARY: {
            finish: function(n) {
                var op = n.type;
                if (n.type != INCREMENT && n.type != DECREMENT)
                    return;

                var join = this.join;
                var binds = this.binds;
                if (!(n[0].type == IDENTIFIER && binds.hasCurrent(n[0].value)))
                    return;

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                var name = n[0].value;
                var c = binds.current(name);
                
                if (binds.isWith && c.type == VAR)
                    return;

                var t = n.tokenizer;
                var ptmp = this.genPostfixSym();

                if (n.postfix) {
                    binds.block.push(mkDecl(this, "VAR", t, ptmp));
                }

                var n2 = mkRawIdentifier(t, name, null, true);
                if (c) {
                    n2.setForward(c.def);
                }

                if (join) {
                    join.rememberUse(name, n2);
                }

                if (n.postfix) {
                    n.parenthesized = true;
                    n.type = COMMA;
                    n.length = 0;
                    n.push(mkAssignSimple(this, t, ptmp,
                                          mkIdentifier(this, t, name)));
                }

                var aop = op == INCREMENT ? PLUS : MINUS;
                var assign = mkAssignSimple(this, t, name,
                                            new Node(t, aop, n2,
                                                     mkNumber(t, 1, true)));

                if (n.postfix) {
                    n.push(assign);
                    n.push(mkIdentifier(this, t, ptmp));
                } else {
                    n.type = ASSIGN;
                    n.length = 0;
                    n.push(assign[0]);
                    n.push(assign[1]);
                }
            }
        },

        MEMBER: {
            build: function(t, tt) {
                this.binds.inRHS++;
                return new Node(t, tt);
            },

            finish: function(n) {
                
                
                
                
                
                
                
                
                
                var join = this.join;
                var binds = this.binds;
                var fb = binds.nearestFunction;

                if (--binds.inRHS > 0) {
                    if (unionOnRight) {
                        n.upvars = n[1].upvars;
                    } else {
                        n.upvars = n[0].upvars;
                    }
                }

                if (n.type != CALL)
                    return;

                function processUpvars(upvars) {
                    var upvarInts = upvars.transClosureI(new Map);
                    var fiuvs = upvars.flowIUVs.members;
                    for (var i in fiuvs) {
                        binds.addUpvar("intervenes", fiuvs[i]);
                    }

                    
                    
                    
                    
                    
                    
                    
                    if (upvars.isEval) {
                        escapeEval(join, binds);
                    } else {
                        var escs;
                        if (escs = upvars.transClosureE(new Map)) {
                            if (escs.length > 0 || upvars.needsEscape) {
                                escapeVars0(join, binds, escs, new Map);
                            }
                        }
                    }

                    var escaped = fb.escapedVars();
                    if (escaped && upvarInts && upvars.needsEscape) {
                        upvarInts.unionWith(escaped);
                    }

                    
                    
                    fb.upvars.unionWith(upvars);

                    if (join) {
                        join.intervene(binds, upvarInts);
                    } else {
                        binds.intervene(upvarInts);
                    }

                    return escaped;
                }

                
                
                
                
                
                var inners = this.binds.inners;
                var base = baseOfCall(n[0]);
                var target = targetOfCall(n[0], IDENTIFIER);

                if (target == "eval") {
                    escapeEval(join, binds);
                } else if (base.type == IDENTIFIER) {
                    var name = base.value;
                    var c = binds.current(name);
                    var uv;

                    if (!c && (uv = binds.upvar(name))) {
                        c = uv;

                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        if (!base.forward) {
                            binds.addUpvar("intervenes", uv);
                        }
                    }

                    if (c) {
                        var upvars = c.upvars || new Upvars;
                        var escaped = processUpvars(upvars);

                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        var uvuses = upvars.uses.members;
                        if (upvars.uses.length > 0) {
                            binds.addBackpatchUpvars(upvars);
                        }

                        for (var i in uvuses) {
                            var use = uvuses[i];
                            var name = use.name;

                            
                            if (fb.evalEscaped ||
                                (escaped && escaped.lookup(use))) {
                                upvars.uses.remove(use);
                                upvars.useTuples.remove(use);
                                continue;
                            }

                            if (binds.current(name) === use) {
                                
                                
                                var useTuple = upvars.useTuples.lookup(use);
                                if (useTuple.cdef === undefined) {
                                    
                                    useTuple.cdef = use.def;
                                } else if (useTuple.cdef !== use.def) {
                                    
                                    upvars.uses.remove(use);
                                    upvars.useTuples.remove(use);
                                }
                            } else if (binds.upvar(name) === use) {
                                
                                
                                
                                
                                if (hasOwnProperty.call(fb.upvarOlds, use.name)) {
                                    upvars.uses.remove(use);
                                    upvars.useTuples.remove(use);
                                }
                            }
                        }
                    }
                } else if (base.upvars) {
                    processUpvars(base.upvars);
                }

                var unionOnRight = n.type == CALL || n.type == NEW_WITH_ARGS ||
                                   n.type == INDEX;
                if (unionOnRight) {
                    escapeVars(join, binds, n[1].upvars || new Upvars);
                }
            }
        },

        PRIMARY: {
            finish: function(n) {
                if (n.type != IDENTIFIER)
                    return;

                var binds = this.binds, join = this.join;
                var fb = binds.nearestFunction;
                var name = n.value;
                var c = binds.current(name);
                var uv = binds.upvar(name);
                var hp = binds.hasParam(name);
                var hup = binds.hasUpParam(name);
                var mb = fb.mu(name);

                
                if (binds.isWith && c && c.type == VAR)
                    return;

                c = upvarTreatedAsLocal(binds, name, c, uv);

                if (c) {
                    n.setForward(c.def, uv === c);
                } else if (mb && (!uv && !hup && !hp)) {
                    
                    
                    mb.pushMuUse(n);
                }

                




                if (!this.secondPass && !binds.isFunction &&
                    !n.internal && !binds.currents[name]) {
                    binds.rememberPossibleHoist(name);
                }

                if (join) {
                    join.rememberUse(name, n);
                }

                
                
                
                
                
                
                if (binds.inRHS > 0) {
                    if (c && !uv) {
                        n.upvars = c.upvars;
                    } else if (uv) {
                        n.upvars = new Upvars;
                        n.upvars.flowIUVs.insert1(uv);
                    }
                }

                if (!c && !uv) {
                    if (!this.secondPass && !n.internal &&
                        !uv && !hp && !hup) {
                        
                        
                        
                        
                        fb.rememberPossibleHoist(name);
                    }

                    binds.closureNeedsEscape();

                    var escaped = fb.escapedVars();
                    if (join) {
                        
                        
                        
                        
                        
                        
                        
                        
                        var tryJoin = join.nearestTryJoin();
                        if (tryJoin && name != "eval") {
                            
                            
                            
                            
                            
                            
                            
                            var ps = join.unionPhisUpTo(tryJoin);
                            var tryBinds = tryJoin.binds;
                            var old;
                            for (var x in ps) {
                                c = tryBinds.current(x);
                                old = ps[x].old;
                                tryJoin.insertPhi(c.type, x, c.def, c.upvars,
                                                  old.def, old.upvars);
                            }
                            binds.closureNeedsEscape();
                            var intervened = join.intervene(binds, escaped);
                            tryJoin.insertDashes(binds, intervened);
                            tryJoin.finishBranch();
                            if (!tryJoin.isFinally) {
                                tryJoin.parent.maybeThrows = true;
                            }
                        } else {
                            join.intervene(binds, escaped);
                        }
                    } else {
                        binds.intervene(escaped);
                    }
                }
            }
        },

        PROPERTY_INIT: {
            build: function(t) {
                var n = new Node(t, PROPERTY_INIT);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            finish: function(n) {
                n.rhsUnionUpvars(n[1]);
            }
        },

        COMMA: {
            build: function(t) {
                var n = new Node(t, COMMA);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            addOperand: function(n, n2) {
                n.rhsUnionUpvars(n2);
                n.push(n2);
            }
        },

        LIST: {
            build: function(t) {
                var n = new Node(t, LIST);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            addOperand: function(n, n2) {
                n.rhsUnionUpvars(n2);
                n.push(n2);
            }
        },

        ARRAY_INIT: {
            build: function(t) {
                var n = new Node(t, ARRAY_INIT);
                n.rhsNewUpvars(this.binds);

                return n;
            },

            addElement: function(n, n2) {
                n.rhsUnionUpvars(n2);
                n.push(n2);
            }
        },

        OBJECT_INIT: {
            build: function(t) {
                var n = new Node(t, OBJECT_INIT);
                n.rhsNewUpvars(this.binds);
                return n;
            },

            addProperty: function(n, n2) {
                n.rhsUnionUpvars(n2);
                n.push(n2);
            }
        },

        WITH: {
            setObject: function(n, e) {
                
                n.object = e;
                
                
                
                
                this.binds = new Bindings(this.binds, false, true);
                this.binds.noPop = true;
                this.noBindingsOnNextBlock = true;
            },

            finish: function(n) {
                var binds = this.binds;
                var join = this.join;

                if (join) {
                    join.intervene(binds, binds.withIntervenes);
                } else {
                    binds.intervene(binds.withIntervenes);
                }
                this.binds = this.binds.parent;
            }
        },

        genDestructuringSym: function() {
            return "$dtmp" + this.destructuringTmpFresh++;
        },

        genPostfixSym: function() {
            return "$ptmp" + this.postfixTmpFresh++;
        },

        setHoists: function(id, vds) {
            this.hoists[id] = vds;
        }
    };

    extendBuilder(SSABuilder, parser.DefaultBuilder);

    



    var Sbp = SSABuilder.prototype;

    Sbp.FUNCTION.build = function(t) {
        var binds = this.binds;
        var n = new Node(t);
        if (n.type != FUNCTION)
            n.type = (n.value == "get") ? GETTER : SETTER;
        n.params = [];

        
        
        this.joinStack.push(this.join);
        this.join = null;
        this.binds = new Bindings(binds, true, false);
        this.binds.noPop = true;
        this.noBindingsOnNextBlock = true;

        return n;
    };

    



    function escapeEval(join, binds) {
        if (join) {
            var intervened = join.intervene(binds);
            var tryJoin = join.nearestTryJoin();
            if (tryJoin) {
                tryJoin.insertDashes(binds, intervened);
                tryJoin.finishBranch();
            }
        } else {
            binds.intervene();
        }

        
        binds.nearestFunction.evalEscaped = true;
        binds.closureIsEval();
    }

    function escapeVars0(join, binds, mayEscape, escapedIntervenes) {
        var fb = binds.nearestFunction;
        var escaped = fb.evalEscaped ? null : fb.escaped;

        binds.closureNeedsEscape();

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (escaped) {
            escaped.defs.unionWith(mayEscape);
            escaped.intervenes.unionWith(escapedIntervenes);
            etci = escaped.transClosureI(new Map);
        } else {
            etci = null;
        }

        if (join) {
            join.intervene(binds, etci);
        } else {
            binds.intervene(etci);
        }
    }

    function escapeVars(join, binds, upvars) {
        
        
        var fb = binds.nearestFunction;
        fb.upvars.unionWith(upvars);

        
        upvars.uses.clear();
        upvars.useTuples.clear();

        if (upvars.isEval) {
            escapeEval(join, binds);
        } else {
            var tci;
            if (tci = upvars.transClosureI(new Map)) {
                escapeVars0(join, binds, tci, upvars.intervenes);
            } else {
                escapeEval(join, binds);
            }
        }

        var fiuvs = upvars.flowIUVs.members;
        for (var i in fiuvs) {
            binds.addUpvar("escapes", fiuvs[i])
        }
    }

    function unionPhiUpvars(operands) {
        var upvars = new Upvars;
        var puv, os;
        for (var i = 0, j = operands.length; i < j; i++) {
            os = operands[i];
            if (os) {
                if (puv = os.upvars) {
                    upvars.unionWith(puv);
                }
            }
        }

        return upvars;
    }

    function fallthroughJoin(n, join, binds) {
        var ps = join.phis, ps2 = {}, e, r;

        function go(x, rhs) {
            var upvars = unionPhiUpvars(ps[x]);
            binds.update(x, rhs, upvars);
        }

        
        
        
        
        
        
        
        
        
        
        var c;
        for (var px in ps) {
            c = binds.current(px);
            ps2[px] = [];
            ps2[px][1] = { def: c.def, upvars: c.upvars };
            ps2[px].old = ps[px].old;
        }
        e = SSAJoin.mkJoin(ps2, join.parent, 2, go);
        r = new Node({ lineno: n.lineno }, SEMICOLON);
        r.expression = e;
        return r;
    }

    function baseOfCall(n) {
        switch (n.type) {
          case DOT:
            return baseOfCall(n[0]);
          case INDEX:
            return baseOfCall(n[0]);
          default:
            return n;
        }
    }

    function targetOfCall(n, ident) {
        switch (n.type) {
          case ident:
            return n.value;
          case DOT:
            return targetOfCall(n[1], IDENTIFIER);
          case INDEX:
            return targetOfCall(n[1], STRING);
          default:
            return null;
        }
    }

    function mkHoistDecl(builder, n) {
        var bDecl = builder.DECL;
        var hoistDecl = bDecl.build(n.tokenizer, n.type);

        bDecl.setName(hoistDecl, n.name);
        
        hoistDecl.initializer = n.initializer;
        bDecl.setReadOnly(n, n.readOnly);
        bDecl.finish(hoistDecl);

        return hoistDecl;
    }

    function mkIndex(builder, t, rhs, x) {
        var bMember = builder.MEMBER;
        var bPrimary = builder.PRIMARY;
        var idx, idxLit;

        idxLit = bPrimary.build(t, STRING)
        idxLit.value = x;
        bPrimary.finish(idxLit);
        idx = bMember.build(t, INDEX);
        bMember.addOperand(idx, rhs);
        bMember.addOperand(idx, idxLit);
        bMember.finish(idx);

        return idx;
    }

    function mkIdentifier(builder, t, name, isExternal) {
        var bPrimary = builder.PRIMARY;
        var n = bPrimary.build(t, IDENTIFIER);
        n.name = n.value = name;
        n.internal = !isExternal;
        bPrimary.finish(n);

        return n;
    }

    function mkAssign(builder, t, lhs, initializer) {
        var bAssign = builder.ASSIGN;

        var n = bAssign.build(t);
        bAssign.addOperand(n, lhs);
        bAssign.addOperand(n, initializer);
        bAssign.finish(n);

        return n;
    }

    function mkAssignSimple(builder, t, name, initializer, isExternal) {
        var bPrimary = builder.PRIMARY;
        var id = bPrimary.build(t, IDENTIFIER);
        id.name = id.value = name;
        id.internal = !isExternal;
        bPrimary.finish(id);
        return mkAssign(builder, t, id, initializer);
    }

    function desugarDestructuringAssign(builder, comma, n, e) {
        var t = n.tokenizer;
        var binds = builder.binds;
        var bComma = builder.COMMA;

        function go(lhss, rhs) {
            var lhs, lhsType, idx;
            for (var x in lhss) {
                lhs = lhss[x];
                lhsType = lhs.type;
                idx = mkIndex(builder, t, rhs, x);
                if (lhsType == IDENTIFIER) {
                    var name = lhs.value;
                    var a = mkAssignSimple(builder, t, name, idx, true);
                    bComma.addOperand(comma, a);
                } else if (lhsType == ARRAY_INIT || lhsType == OBJECT_INIT) {
                    
                    
                    go(lhs, idx);
                } else {
                    var a = mkAssign(builder, t, lhs, idx);
                    bComma.addOperand(comma, a);
                }
            }
        }

        
        var decl = mkDecl(builder, "LET", n.tokenizer,
                          builder.genDestructuringSym(),
                          e, false);
        builder.binds.block.push(decl);
        decl[0].setForward(e);

        go(n.destructuredNames, decl[0]);
    }

    function desugarDestructuringInit(builder, n, e) {
        var t = n.tokenizer;
        var binds = builder.binds;
        var ddecls = n.destructuredDecls;
        var bDecl = builder.DECL;

        function go(ddecls, rhs) {
            var n2, n3, id, idxLit, idx;
            for (var x in ddecls) {
                n2 = ddecls[x];

                
                idx = mkIndex(builder, t, rhs, x);

                if (n2.type == IDENTIFIER) {
                    bDecl.setInitializer(n2, idx);
                    bDecl.finish(n2);
                } else {
                    go(n2, idx);
                }
            }
        }

        
        
        
        var block = builder.binds.block;
        if (block) {
            var dtmp = builder.genDestructuringSym();
            var decl = mkDecl(builder, "LET", t, dtmp, e, false);
            block.push(decl);
            decl[0].setForward(e);
            go(ddecls, decl[0]);
        } else {
            
            
            
            go(ddecls, e);
        }

    }

    function mkAddDestructuringDecl(type, ro) {
        return function(n, n2, x) {
            var t = n2.tokenizer;
            var binds = this.binds;
            var bDecl = this.DECL;
            var numDecls = 0;
            var b;
            if (type == VAR) {
                b = this.VAR;
            } else if (type == CONST) {
                b = this.CONST;
            } else {
                b = this.LET;
            }

            function go(lhss) {
                var ddecls = {};

                for (var idx in lhss) {
                    var lhs = lhss[idx];
                    if (lhs.type == IDENTIFIER) {
                        var decl = bDecl.build(t);
                        var name = lhs.value;
                        decl.forward = lhs.forward;

                        
                        
                        bDecl.setName(decl, name);
                        bDecl.setReadOnly(decl, ro);
                        
                        
                        decl.wasLocal = binds.hasCurrent(name);
                        b.addDecl(n, decl, x);
                        ddecls[idx] = decl;
                        numDecls++;
                    } else {
                        ddecls[idx] = go(lhs);
                    }
                }

                return ddecls;
            }

            n2.destructuredDecls = go(n2.name.destructuredNames);
            n2.numDecls = numDecls;
        }
    }

    function mkDecl(builder, tt, t, name, initializer, isExternal) {
        var b = builder[tt];
        var bDecl = builder.DECL;

        var decl = bDecl.build(t);
        bDecl.setName(decl, name);
        decl.internal = !isExternal;
        var lt = b.build(t);
        b.addDecl(lt, decl);

        if (initializer) {
            bDecl.setInitializer(decl, initializer);
        }

        bDecl.finish(decl);
        b.finish(lt);

        return lt;
    }

    function mkCall(f, n, isExternal) {
        var ident = new Node(n.tokenizer, IDENTIFIER);
        ident.value = f;
        ident.internal = !isExternal;
        return new Node(n.tokenizer, CALL, ident,
                        new Node(n.tokenizer, LIST, n));
    }

    function mkRawIdentifier(t, name, init, isExternal) {
        var n = new Node(t, IDENTIFIER);
        n.name = n.value = name;
        n.internal = !isExternal;
        n.initializer = init;
        return n;
    }

    function mkNumber(t, i, isExternal) {
        var n = new Node(t, NUMBER);
        n.value = i;
        n.internal = !isExternal;
        return n;
    }

    var Np = Node.prototype;

    Np.rhsNewUpvars = function(binds) {
        if (binds.inRHS > 0) {
            this.upvars = new Upvars;
        }
    };

    Np.rhsUnionUpvars = function(n) {
        if (this.upvars && n.upvars) {
            this.upvars.unionWith(n.upvars)
        }
    };

    function upvarTreatedAsLocal(binds, name, c, uv) {
        var fb = binds.nearestFunction;
        
        
        return hasOwnProperty.call(fb.upvarOlds, name) ? uv : c;
    }

    Np.setForward = function(fwd, isUpvar) {
        
        if (fwd === dash)
            return;

        this.forward = fwd;
        if (fwd) {
            if (!fwd.backwards)
                fwd.backwards = [];

            fwd.backwards.push(this);
        }
    };

    Np.resolve = function() {
        var f = this.forward;
        if (f) {
            this.forward = f.resolve();
            return this.forward;
        }
        return this;
    };

    
    
    Np.pushPhiUse = function(p) {
        if (!this.phiUses)
            this.phiUses = [];

        this.phiUses.push(p);
    };

    parser.SSABuilder = SSABuilder;

}());
