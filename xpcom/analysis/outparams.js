require({ version: '1.8' });
require({ after_gcc_pass: 'cfg' });

include('treehydra.js');

include('util.js');
include('gcc_util.js');
include('gcc_print.js');
include('unstable/adts.js');
include('unstable/analysis.js');
include('unstable/esp.js');
let Zero_NonZero = {};
include('unstable/zero_nonzero.js', Zero_NonZero);

include('xpcom/analysis/mayreturn.js');

function safe_location_of(t) {
  if (t === undefined)
    return UNKNOWN_LOCATION;
  
  return location_of(t);
}

MapFactory.use_injective = true;


let TRACE_FUNCTIONS = 0;

let TRACE_ESP = 0;

let TRACE_CALL_SEM = 0;

let TRACE_PERF = 0;

let LOG_RESULTS = false;

const WARN_ON_SET_NULL = false;
const WARN_ON_SET_FAILURE = false;


let func_filter;
if (this.arg == undefined || this.arg == '') {
  func_filter = function(fd) true;
} else {
  func_filter = function(fd) function_decl_name(fd) == this.arg;
}

function process_tree(func_decl) {
  if (!func_filter(func_decl)) return;

  
  if (is_constructor(func_decl)) return;
  let psem = OutparamCheck.prototype.func_param_semantics(func_decl);
  if (!psem.some(function(x) x.check)) return;
  let decl = rectify_function_decl(func_decl);
  if (decl.resultType != 'nsresult' && decl.resultType != 'PRBool' &&
      decl.resultType != 'void') {
    warning("Cannot analyze outparam usage for function with return type '" +
            decl.resultType + "'", location_of(func_decl));
    return;
  }

  let params = [ v for (v in flatten_chain(DECL_ARGUMENTS(func_decl))) ];
  let outparam_list = [];
  let psem_list = [];
  for (let i = 0; i < psem.length; ++i) {
    if (psem[i].check) {
      outparam_list.push(params[i]);
      psem_list.push(psem[i]);
    }
  }
  if (outparam_list.length == 0) return;

  
  let fstring = rfunc_string(decl);
  if (TRACE_FUNCTIONS) {
    print('* function ' + fstring);
    print('    ' + loc_string(location_of(func_decl)));
  }
  if (TRACE_PERF) timer_start(fstring);
  for (let i = 0; i < outparam_list.length; ++i) {
    let p = outparam_list[i];
    if (TRACE_FUNCTIONS) {
      print("  outparam " + expr_display(p) + " " + DECL_UID(p) + ' ' + 
            psem_list[i].label);
    }
  }

  let cfg = function_decl_cfg(func_decl);

  let [retvar, retvars] = function() {
    let trace = 0;
    let a = new MayReturnAnalysis(cfg, trace);
    a.run();
    return [a.retvar, a.vbls];
  }();
  if (retvar == undefined && decl.resultType != 'void') throw new Error("assert");

  {
    let trace = TRACE_ESP;
    for (let i = 0; i < outparam_list.length; ++i) {
      let psem = [ psem_list[i] ];
      let outparam = [ outparam_list[i] ];
      let a = new OutparamCheck(cfg, psem, outparam, retvar, retvars, trace);
      
      a.fndecl = func_decl;
      a.run();
      a.check(decl.resultType == 'void', func_decl);
    }
  }
  
  if (TRACE_PERF) timer_stop(fstring);
}

function is_constructor(function_decl)
{
  return function_decl.decl_common.lang_specific.decl_flags.constructor_attr;
}


function OutparamCheck(cfg, psem_list, outparam_list, retvar, retvar_set, 
                       trace) {
  
  
  this.retvar_set = retvar_set;
  this.retvar = retvar;

  
  this.outparam_list = outparam_list
  this.outparams = create_decl_set(outparam_list);
  this.psem_list = psem_list;

  
  let psvar_list = [];
  for each (let v in outparam_list) {
    psvar_list.push(new ESP.PropVarSpec(v, true, av.NOT_WRITTEN));
  }
  for (let v in retvar_set.items()) {
    psvar_list.push(new ESP.PropVarSpec(v, v == this.retvar, ESP.TOP));
  }
  if (trace) {
    print("PS vars");
    for each (let v in this.psvar_list) {
      print("    " + expr_display(v.vbl));
    }
  }
  this.zeroNonzero = new Zero_NonZero.Zero_NonZero();
  ESP.Analysis.call(this, cfg, psvar_list, av.meet, trace);
}


function AbstractValue(name, ch) {
  this.name = name;
  this.ch = ch;
}

AbstractValue.prototype.equals = function(v) {
  return this === v;
}

AbstractValue.prototype.toString = function() {
  return this.name + ' (' + this.ch + ')';
}

AbstractValue.prototype.toShortString = function() {
  return this.ch;
}

let avspec = [
  
  [ 'NULL',          'x' ],   
  [ 'NOT_WRITTEN',   '-' ],   
  [ 'WROTE_NULL',    '/' ],   
  [ 'WRITTEN',       '+' ],   
  
  
  
  
  
  [ 'MAYBE_WRITTEN', '?' ],   
];

let av = {};
for each (let [name, ch] in avspec) {
  av[name] = new AbstractValue(name, ch);
}

av.ZERO = Zero_NonZero.Lattice.ZERO;
av.NONZERO = Zero_NonZero.Lattice.NONZERO;


















let cachedAVs = {};




function makeOutparamAV(v) {
  let key = 'outparam_' + DECL_UID(v);
  if (key in cachedAVs) return cachedAVs[key];

  let ans = cachedAVs[key] = 
    new AbstractValue('OUTPARAM:' + expr_display(v), 'P');
  ans.outparam = v;
  return ans;
}


av.intVal = function(v) {
  if (v.hasOwnProperty('int_val'))
    return v.int_val;
  return undefined;
}


av.meet = function(v1, v2) {
  
  let values = [v1,v2]
  if (values.indexOf(av.LOCKED) != -1
      || values.indexOf(av.UNLOCKED) != -1)
    return ESP.NOT_REACHED;

  return Zero_NonZero.meet(v1, v2)
};


OutparamCheck.prototype = new ESP.Analysis;

OutparamCheck.prototype.split = function(vbl, v) {
  
  if (v != ESP.TOP) throw new Error("not implemented");
  return [ av.ZERO, av.NONZERO ];
}

OutparamCheck.prototype.updateEdgeState = function(e) {
  e.state.keepOnly(e.dest.keepVars);
}

OutparamCheck.prototype.flowState = function(isn, state) {
  switch (TREE_CODE(isn)) {
  case GIMPLE_MODIFY_STMT:
    this.processAssign(isn, state);
    break;
  case CALL_EXPR:
    this.processCall(undefined, isn, isn, state);
    break;
  case SWITCH_EXPR:
  case COND_EXPR:
    
    break;
  default:
    this.zeroNonzero.flowState(isn, state);
  }
}

OutparamCheck.prototype.flowStateCond = function(isn, truth, state) {
  this.zeroNonzero.flowStateCond(isn, truth, state);
}



OutparamCheck.prototype.processAssign = function(isn, state) {
  let lhs = isn.operands()[0];
  let rhs = isn.operands()[1];

  if (DECL_P(lhs)) {
    
    if (TREE_CODE(rhs) == NOP_EXPR) {
      rhs = rhs.operands()[0];
    }

    if (DECL_P(rhs) && this.outparams.has(rhs)) {
        
        
        state.assignValue(lhs, makeOutparamAV(rhs), isn);
        return;
    }

    
    
    switch (TREE_CODE(rhs)) {
    case INTEGER_CST:
      if (this.outparams.has(lhs)) {
        warning("assigning to outparam pointer");
        return;
      }
      break;
    case EQ_EXPR: {
      
      let [op1, op2] = rhs.operands();
      if (DECL_P(op1) && this.outparams.has(op1) && expr_literal_int(op2) == 0) {
        state.update(function(ss) {
          let [s1, s2] = [ss, ss.copy()]; 
          s1.assignValue(lhs, av.NONZERO, isn);
          s1.assignValue(op1, av.NULL, isn);
          s2.assignValue(lhs, av.ZERO, isn);
          return [s1, s2];
        });
        return;
      }
    }
      break;
    case CALL_EXPR:
      let fname = call_function_name(rhs);
      if (fname == 'NS_FAILED') {
        this.processTest(lhs, rhs, av.NONZERO, isn, state);
      } else if (fname == 'NS_SUCCEEDED') {
        this.processTest(lhs, rhs, av.ZERO, isn, state);
      } else if (fname == '__builtin_expect') {
        
        state.assign(lhs, call_args(rhs)[0], isn);
      } else {
        this.processCall(lhs, rhs, isn, state);
      }
      return;

    case INDIRECT_REF:
      
      
      let v = rhs.operands()[0];
      if (DECL_P(v) && this.outparams.has(v) && 
          TREE_CODE(TREE_TYPE(v)) == POINTER_TYPE) {
        state.update(function(ss) {
          let val = ss.get(v) == av.WROTE_NULL ? av.ZERO : av.NONZERO;
          ss.assignValue(lhs, val, isn);
          return [ ss ];
        });
        return;
      }
    }

    
    this.zeroNonzero.processAssign(isn, state);
    return;
  }

  switch (TREE_CODE(lhs)) {
  case INDIRECT_REF:
    
    
    let e = TREE_OPERAND(lhs, 0);
    if (this.outparams.has(e)) {
      if (expr_literal_int(rhs) == 0) {
        state.assignValue(e, av.WROTE_NULL, isn);
      } else if (DECL_P(rhs)) {
        state.update(function(ss) {
          let [s1, s2] = [ss.copy(), ss]; 
          s1.assignValue(e, av.WROTE_NULL, isn);
          s1.assignValue(rhs, av.ZERO, isn);
          s2.assignValue(e, av.WRITTEN, isn);
          s2.assignValue(rhs, av.NONZERO, isn);
          return [s1,s2];
        });
      } else {
        state.assignValue(e, av.WRITTEN, isn);
      }
    } else {
      
    }
    break;
  case COMPONENT_REF: 
  case ARRAY_REF: 
  case EXC_PTR_EXPR:
  case FILTER_EXPR:
    break;
  default:
    print(TREE_CODE(lhs));
    throw new Error("ni");
  }
}


OutparamCheck.prototype.processTest = function(lhs, call, val, blame, state) {
  let arg = call_arg(call, 0);
  if (DECL_P(arg)) {
    this.zeroNonzero.predicate(state, lhs, val, arg, blame);
  } else {
    state.assignValue(lhs, ESP.TOP, blame);
  }
};


OutparamCheck.prototype.processCall = function(dest, expr, blame, state) {
  let args = call_args(expr);
  let callable = callable_arg_function_decl(CALL_EXPR_FN(expr));
  let psem = this.func_param_semantics(callable);

  if (TRACE_CALL_SEM) {
    print("param semantics:" + psem);
  }

  if (args.length != psem.length) {
    let ct = TREE_TYPE(callable);
    if (TREE_CODE(ct) == POINTER_TYPE) ct = TREE_TYPE(ct);
    if (args.length < psem.length || !stdarg_p(ct)) {
      let name = function_decl_name(callable);
      
      if (name != 'operator new' && name != 'operator delete' &&
          name != 'operator new []' && name != 'operator delete []' &&
          name.substr(0, 5) != '__cxa' &&
          name.substr(0, 9) != '__builtin') {
        throw Error("bad len for '" + name + "': " + args.length + ' args, ' + 
                    psem.length + ' params');
      }
    }
  }

  
  let updates = [];
  for (let i = 0; i < psem.length; ++i) {
    let arg = args[i];
    
    
    
    if (TREE_CODE(arg) == ADDR_EXPR) {
      let v = arg.operands()[0];
      if (DECL_P(v) && this.retvar_set.has(v)) {
        dest = v;
      }
    }
    
    
    
    arg = unwrap_outparam(arg, state);
    let sem = psem[i];
    if (sem == ps.CONST) continue;
    
    
    
    if (TREE_CODE(arg) == ADDR_EXPR) {
      let v = arg.operands()[0];
      if (DECL_P(v)) {
        state.remove(v);
      }
    }
    if (!DECL_P(arg) || !this.outparams.has(arg)) continue;
    
    updates.push([arg, sem]);
  }
  
  if (updates.length) {
    if (dest != undefined && DECL_P(dest)) {
      
      let [ succ_ret, fail_ret ] = ret_coding(callable);

      state.update(function(ss) {
        let [s1, s2] = [ss.copy(), ss]; 
        for each (let [vbl, sem] in updates) {
          s1.assignValue(vbl, sem.val, blame);
          s1.assignValue(dest, succ_ret, blame);
        }
        s2.assignValue(dest, fail_ret, blame);
        return [s1,s2];
      });
    } else {
      
      
      
      
      state.update(function(ss) {
        for each (let [vbl, sem] in updates) {
          if (sem == ps.OUTNOFAIL || sem == ps.OUTNOFAILNOCHECK) {
            ss.assignValue(vbl, av.WRITTEN, blame);
            return [ss];
          } else {
            let [s1, s2] = [ss.copy(), ss]; 
            for each (let [vbl, sem] in updates) {
              s1.assignValue(vbl, sem.val, blame);
            }
            return [s1,s2];
          }
        }
      });
    }
  } else {
    
    if (dest != undefined && DECL_P(dest)) {
      state.remove(dest, blame);
    }
  }
};




function ret_coding(callable) {
  let type = TREE_TYPE(callable);
  if (TREE_CODE(type) == POINTER_TYPE) type = TREE_TYPE(type);

  let rtname = TYPE_NAME(TREE_TYPE(type));
  if (rtname && IDENTIFIER_POINTER(DECL_NAME(rtname)) == 'PRBool') {
    return [ av.NONZERO, av.ZERO ];
  } else {
    return [ av.ZERO, av.NONZERO ];
  }
}

function unwrap_outparam(arg, state) {
  if (!DECL_P(arg) || state.factory.outparams.has(arg)) return arg;

  let outparam;
  for (let ss in state.substates.getValues()) {
    let val = ss.get(arg);
    if (val != undefined && val.hasOwnProperty('outparam')) {
      outparam = val.outparam;
    }
  }
  if (outparam) return outparam;
  return arg;
}


OutparamCheck.prototype.check = function(isvoid, fndecl) {
  let state = this.cfg.x_exit_block_ptr.stateOut;
  for (let substate in state.substates.getValues()) {
    this.checkSubstate(isvoid, fndecl, substate);
  }
}

OutparamCheck.prototype.checkSubstate = function(isvoid, fndecl, ss) {
  if (isvoid) {
    this.checkSubstateSuccess(ss);
  } else {
    let [succ, fail] = ret_coding(fndecl);
    let rv = ss.get(this.retvar);
    
    
    if (av.meet(rv, succ) == rv) {
      this.checkSubstateSuccess(ss);
    } else if (av.meet(rv, fail) == rv) {
      this.checkSubstateFailure(ss);
    } else {
      
      
      warning("Outparams checker cannot determine rv success/failure",
              location_of(fndecl));
      this.checkSubstateSuccess(ss);
      this.checkSubstateFailure(ss);
    }
  }
}





OutparamCheck.prototype.findReturnStmt = function(ss) {
  if (this.retvar != undefined)
    return ss.getBlame(this.retvar);

  if (this.cfg._cached_return)
    return this.cfg._cached_return;
  
  for (let bb in cfg_bb_iterator(this.cfg)) {
    for (let isn in bb_isn_iterator(bb)) {
      if (TREE_CODE(isn) == RETURN_EXPR) {
        return this.cfg._cached_return = isn;
      }
    }
  }

  return undefined;
}

OutparamCheck.prototype.checkSubstateSuccess = function(ss) {
  for (let i = 0; i < this.psem_list.length; ++i) {
    let [v, psem] = [ this.outparam_list[i], this.psem_list[i] ];
    if (psem == ps.INOUT) continue;
    let val = ss.get(v);
    if (val == av.NOT_WRITTEN) {
      this.logResult('succ', 'not_written', 'error');
      this.warn([this.findReturnStmt(ss), "outparam '" + expr_display(v) + "' not written on NS_SUCCEEDED(return value)"],
                [v, "outparam declared here"]);
    } else if (val == av.MAYBE_WRITTEN) {
      this.logResult('succ', 'maybe_written', 'error');

      let blameStmt = ss.getBlame(v);
      let callMsg;
      let callName = "";
      try {
        let callExpr = blameStmt.tree_check(GIMPLE_MODIFY_STMT).
          operands()[1].tree_check(CALL_EXPR);
        let callDecl = callable_arg_function_decl(CALL_EXPR_FN(callExpr));
        
        callMsg = [callDecl, "declared here"];
        callName = " '" + decl_name(callDecl) + "'";
      }
      catch (e if e.TreeCheckError) { }
      
      this.warn([this.findReturnStmt(ss), "outparam '" + expr_display(v) + "' not written on NS_SUCCEEDED(return value)"],
                [v, "outparam declared here"],
                [blameStmt, "possibly written by unannotated function call" + callName],
                callMsg);
    } else {
      this.logResult('succ', '', 'ok');
    }
  }    
}

OutparamCheck.prototype.checkSubstateFailure = function(ss) {
  for (let i = 0; i < this.psem_list.length; ++i) {
    let [v, ps] = [ this.outparam_list[i], this.psem_list[i] ];
    let val = ss.get(v);
    if (val == av.WRITTEN) {
      this.logResult('fail', 'written', 'error');
      if (WARN_ON_SET_FAILURE) {
        this.warn([this.findReturnStmt(ss), "outparam '" + expr_display(v) + "' written on NS_FAILED(return value)"],
                  [v, "outparam declared here"],
                  [ss.getBlame(v), "written here"]);
      }
    } else if (val == av.WROTE_NULL) {
      this.logResult('fail', 'wrote_null', 'warning');
      if (WARN_ON_SET_NULL) {
        this.warn([this.findReturnStmt(ss), "NULL written to outparam '" + expr_display(v) + "' on NS_FAILED(return value)"],
                  [v, "outparam declared here"],
                  [ss.getBlame(v), "written here"]);
      }
    } else {
      this.logResult('fail', '', 'ok');
    }
  }    
}




OutparamCheck.prototype.warn = function(arg0) {
  let loc = safe_location_of(arg0[0]);
  let msg = arg0[1];

  for (let i = 1; i < arguments.length; ++i) {
    if (arguments[i] === undefined) continue;
    let [atree, amsg] = arguments[i];
    msg += "\n" + loc_string(safe_location_of(atree)) + ":   " + amsg;
  }
  warning(msg, loc);
}

OutparamCheck.prototype.logResult = function(rv, msg, kind) {
  if (LOG_RESULTS) {
    let s = [ '"' + x + '"' for each (x in [ loc_string(location_of(this.fndecl)), function_decl_name(this.fndecl), rv, msg, kind ]) ].join(', ');
    print(":LR: (" + s + ")");
  }
}








let ps = {
  OUTNOFAIL: { label: 'out-no-fail', val: av.WRITTEN,  check: true },
  
  
  
  OUTNOFAILNOCHECK: { label: 'out-no-fail-no-check' },
  OUT:       { label: 'out',         val: av.WRITTEN,  check: true },
  INOUT:     { label: 'inout',       val: av.WRITTEN,  check: true },
  MAYBE:     { label: 'maybe',       val: av.MAYBE_WRITTEN},  
  CONST:     { label: 'const' }   
};



OutparamCheck.prototype.func_param_semantics = function(callable) {
  let ftype = TREE_TYPE(callable);
  if (TREE_CODE(ftype) == POINTER_TYPE) ftype = TREE_TYPE(ftype);
  
  let rtype = TREE_TYPE(ftype);
  let nofail = TREE_CODE(rtype) == VOID_TYPE;
  
  let guess = type_string(rtype) == 'nsresult';

  
  let params;     
  let types;      
  let string_mutator = false;
  if (TREE_CODE(callable) == FUNCTION_DECL) {
    params = [ p for (p in function_decl_params(callable)) ];
    types = [ TREE_TYPE(p) for each (p in params) ];
    string_mutator = is_string_mutator(callable);
  } else {
    types = [ p for (p in function_type_args(ftype)) 
                if (TREE_CODE(p) != VOID_TYPE) ];
  }

  
  let ans = [];
  for (let i = 0; i < types.length; ++i) {
    let sem;
    if (i == 0 && string_mutator) {
      
      
      sem = ps.OUTNOFAILNOCHECK;
    } else {
      if (params) sem = decode_attr(DECL_ATTRIBUTES(params[i]));
      if (TRACE_CALL_SEM >= 2) print("param " + i + ": annotated " + sem);
      if (sem == undefined) {
        sem = decode_attr(TYPE_ATTRIBUTES(types[i]));
        if (TRACE_CALL_SEM >= 2) print("type " + i + ": annotated " + sem);
        if (sem == undefined) {
          if (guess && type_is_outparam(types[i])) {
            
            sem = i < types.length - 1 ? ps.MAYBE : ps.OUT;
          } else {
            sem = ps.CONST;
          }
        }
      }
      if (sem == ps.OUT && nofail) sem = ps.OUTNOFAIL;
    }
    if (sem == undefined) throw new Error("assert");
    ans.push(sem);
  }
  return ans;
}






function decode_attr(attrs) {
  
  
  for each (let attr in rectify_attributes(attrs)) {
    if (attr.name == 'user') {
      for each (let arg in attr.args) {
        if (arg == 'NS_outparam') {
          return ps.OUT;
        } else if (arg == 'NS_inoutparam') {
          return ps.INOUT;
        } else if (arg == 'NS_inparam') {
          return ps.CONST;
        }
      }
    }
  }
  return undefined;
}




function type_is_outparam(type) {
  switch (TREE_CODE(type)) {
  case POINTER_TYPE:
    return pointer_type_is_outparam(TREE_TYPE(type));
  case REFERENCE_TYPE:
    let rt = TREE_TYPE(type);
    return !TYPE_READONLY(rt) && is_string_type(rt);
  default:
    
    
    return false;
  }
}



function pointer_type_is_outparam(pt) {
  if (TYPE_READONLY(pt)) return false;

  switch (TREE_CODE(pt)) {
  case POINTER_TYPE:
  case ARRAY_TYPE: {
    
    let ppt = TREE_TYPE(pt);
    let tname = TYPE_NAME(ppt);
    if (tname == undefined) return false;
    let name = decl_name_string(tname);
    return name == 'void' || name == 'char' || name == 'PRUnichar' ||
      name.substr(0, 3) == 'nsI';
  }
  case INTEGER_TYPE: {
    
    
    let name = decl_name_string(TYPE_NAME(pt));
    return name != 'char' && name != 'PRUnichar';
  }
  case ENUMERAL_TYPE:
  case REAL_TYPE:
  case UNION_TYPE:
    return true;
  case RECORD_TYPE:
    
    return false;
  case FUNCTION_TYPE:
  case VOID_TYPE:
    return false;
  default:
    throw new Error("can't guess if a pointer to this type is an outparam: " +
                    TREE_CODE(pt) + ': ' + type_string(pt));
  }
}


let cached_string_types = MapFactory.create_map(
  function (x, y) x == y,
  function (x) x,
  function (t) t,
  function (t) t);




cached_string_types.put('nsAString', true);
cached_string_types.put('nsACString', true);
cached_string_types.put('nsAString_internal', true);
cached_string_types.put('nsACString_internal', true);





function is_string_type(type, binfo) {
  if (TREE_CODE(type) != RECORD_TYPE) return false;
  
  let name = decl_name_string(TYPE_NAME(type));
  let ans = cached_string_types.get(name);
  if (ans != undefined) return ans;

  ans = false;
  binfo = binfo != undefined ? binfo : TYPE_BINFO(type);
  if (binfo != undefined) {
    for each (let base in VEC_iterate(BINFO_BASE_BINFOS(binfo))) {
      let parent_ans = is_string_type(BINFO_TYPE(base), base);
      if (parent_ans) {
        ans = true;
        break;
      }
    }
  }
  cached_string_types.put(name, ans);
  
  return ans;
}

function is_string_ptr_type(type) {
  return TREE_CODE(type) == POINTER_TYPE && is_string_type(TREE_TYPE(type));
}



function is_string_mutator(fndecl) {
  let first_param = function() {
    for (let p in function_decl_params(fndecl)) {
      return p;
    }
    return undefined;
  }();

  return first_param != undefined && 
    decl_name_string(first_param) == 'this' &&
    is_string_ptr_type(TREE_TYPE(first_param)) &&
    !TYPE_READONLY(TREE_TYPE(TREE_TYPE(first_param)));
}

