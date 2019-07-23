require({ version: '1.8' });
require({ after_gcc_pass: 'cfg' });

include('treehydra.js');

include('util.js');
include('gcc_util.js');
include('gcc_print.js');
include('unstable/adts.js');
include('unstable/analysis.js');
include('unstable/esp.js');

include('liveness.js');
include('mayreturn.js');

MapFactory.use_injective = true;

let TRACE_FUNCTIONS = 0;
let TRACE_ESP = 0;
let TRACE_PERF = 0;
let LOG_RESULTS = false;


let func_filter;
if (this.arg == undefined || this.arg == '') {
  func_filter = function(fd) true;
} else {
  func_filter = function(fd) function_decl_name(fd) == this.arg;
}

function process_tree(func_decl) {
  if (!func_filter(func_decl)) return;

  
  let decl = rectify_function_decl(func_decl);
  if (decl.resultType != 'nsresult') return;

  let psem = OutparamCheck.prototype.func_param_semantics(func_decl);
  let params = [ v for (v in flatten_chain(DECL_ARGUMENTS(func_decl))) ];
  let outparam_list = [];
  let psem_list = [];
  for (let i = 0; i < psem.length; ++i) {
    if (psem[i] == ps.OUT || psem[i] == ps.INOUT) {
      outparam_list.push(params[i]);
      psem_list.push(psem[i]);
    }
  }
  if (outparam_list.length == 0) return;

  
  let fstring = rfunc_string(decl);
  if (TRACE_FUNCTIONS) print('* function ' + fstring);
  if (TRACE_PERF) timer_start(fstring);
  for (let i = 0; i < outparam_list.length; ++i) {
    let p = outparam_list[i];
    if (TRACE_FUNCTIONS) {
      print("  outparam " + expr_display(p) + " " + DECL_UID(p) + ' ' + 
            psem_list[i].label);
    }
  }

  let cfg = function_decl_cfg(func_decl);

  {
    let trace = 0;
    let b = new LivenessAnalysis(cfg, trace);
    b.run();
    for (let bb in cfg_bb_iterator(cfg)) {
      bb.liveVarsIn = bb.stateIn;
      bb.liveVarsOut = bb.stateOut;
    }
  }
  
  let [retvar, retvars] = function() {
    let trace = 0;
    let a = new MayReturnAnalysis(cfg, trace);
    a.run();
    return [a.retvar, a.vbls];
  }();
  if (retvar == undefined) throw new Error("assert");

  {
    let trace = TRACE_ESP;
    let fts = link_switches(cfg);
    let a = new OutparamCheck(cfg, psem_list, outparam_list, retvar, retvars, fts, trace);
    
    a.fndecl = func_decl;
    a.run();
    a.check();
  }
  
  if (TRACE_PERF) timer_stop(fstring);
}


function OutparamCheck(cfg, psem_list, outparam_list, retvar, retvar_set, finally_tmps, trace) {
  this.retvar = retvar;
  this.psem_list = psem_list;
  
  this.outparam_list = outparam_list
  this.outparams = create_decl_set(outparam_list);
  this.psvar_list = outparam_list.slice(0);
  for (let v in retvar_set.items()) {
    this.psvar_list.push(v);
  }
  for each (let v in finally_tmps) {
    this.psvar_list.push(v);
  }
  if (trace) {
    print("PS vars");
    for each (let v in this.psvar_list) {
      print("    " + expr_display(v));
    }
  }
  ESP.Analysis.call(this, cfg, this.psvar_list, av.BOTTOM, trace);
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

let avspec = [
  
  [ 'BOTTOM',        '.' ],   

  
  [ 'NULL',          'x' ],   
  [ 'NOT_WRITTEN',   '-' ],   
  [ 'WROTE_NULL',    '/' ],   
  [ 'WRITTEN',       '+' ],   
  
  
  
  
  
  [ 'MAYBE_WRITTEN', '?' ],   
  
  
  [ 'ZERO',          '0' ],   
  [ 'NONZERO',       '1' ]    
];

let av = {};
for each (let [name, ch] in avspec) {
  av[name] = new AbstractValue(name, ch);
}

av.ZERO.negation = av.NONZERO;
av.NONZERO.negation = av.ZERO;

let cachedAVs = {};



function makeIntAV(v) {
  let key = 'int_' + v;
  if (cachedAVs.hasOwnProperty(key)) return cachedAVs[key];

  let s = "" + v;
  let ans = cachedAVs[key] = new AbstractValue(s, s);
  ans.int_val = v;
  return ans;
}




function makeOutparamAV(v) {
  let key = 'outparam_' + DECL_UID(v);
  if (cachedAVs.hasOwnProperty(key)) return cachedAVs[key];

  let ans = cachedAVs[key] = 
    new AbstractValue('OUTPARAM:' + expr_display(v), 'P');
  ans.outparam = v;
  return ans;
}


OutparamCheck.prototype = new ESP.Analysis;

OutparamCheck.prototype.startValues = function() {
  let ans = create_decl_map();
  for each (let p in this.psvar_list) {
    ans.put(p, this.outparams.has(p) ? av.NOT_WRITTEN : av.BOTTOM);
  }
  return ans;
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
  case RETURN_EXPR:
    this.processAssign(isn.operands()[0], state);
    break;
  case LABEL_EXPR:
  case RESX_EXPR:
  case ASM_EXPR:
    
    break;
  default:
    print(TREE_CODE(isn));
    throw new Error("ni");
  }
}

OutparamCheck.prototype.flowStateCond = function(isn, truth, state) {
  switch (TREE_CODE(isn)) {
  case COND_EXPR:
    this.flowStateIf(isn, truth, state);
    break;
  case SWITCH_EXPR:
    this.flowStateSwitch(isn, truth, state);
    break;
  default:
    throw new Error("ni " + TREE_CODE(isn));
  }
}

OutparamCheck.prototype.flowStateIf = function(isn, truth, state) {
  let exp = TREE_OPERAND(isn, 0);

  if (DECL_P(exp)) {
    this.filter(state, exp, av.NONZERO, truth, isn);
    return;
  }

  switch (TREE_CODE(exp)) {
  case EQ_EXPR:
  case NE_EXPR:
    
    let op1 = TREE_OPERAND(exp, 0);
    let op2 = TREE_OPERAND(exp, 1);
    if (expr_literal_int(op1) != undefined) {
      [op1,op2] = [op2,op1];
    }
    if (!DECL_P(op1)) break;
    if (expr_literal_int(op2) != 0) break;
    let val = TREE_CODE(exp) == EQ_EXPR ? av.ZERO : av.NONZERO;

    this.filter(state, op1, val, truth, isn);
    break;
  default:
    
  }

};

OutparamCheck.prototype.flowStateSwitch = function(isn, truth, state) {
  let exp = TREE_OPERAND(isn, 0);

  if (DECL_P(exp)) {
    if (truth != null) {
      this.filter(state, exp, makeIntAV(truth), true, isn);
    }
    return;
  }
  throw new Error("ni");
}



OutparamCheck.prototype.filter = function(state, vbl, val, truth, blame) {
  if (truth != true && truth != false) throw new Error("ni " + truth);
  if (this.outparams.has(vbl)) {
    
    
    if (truth == true && val == av.ZERO || truth == false && val == av.NONZERO) {
      state.assignValue(vbl, av.NULL, blame);
    }
  } else {
    if (truth == false) {
      val = val.negation;
    }
    state.filter(vbl, val, blame);
  }
};

OutparamCheck.prototype.processAssign = function(isn, state) {
  let lhs = isn.operands()[0];
  let rhs = isn.operands()[1];

  if (DECL_P(lhs)) {
    
    if (TREE_CODE(rhs) == NOP_EXPR) {
      rhs = rhs.operands()[0];
    }

    if (DECL_P(rhs)) {
      if (this.outparams.has(rhs)) {
        
        
        state.assignValue(lhs, makeOutparamAV(rhs), isn);
      } else {
        state.assign(lhs, rhs, isn);
      }
      return
    }
    
    switch (TREE_CODE(rhs)) {
    case INTEGER_CST:
      if (this.outparams.has(lhs)) {
        warning("assigning to outparam pointer");
      } else {
        
        if (is_finally_tmp(lhs)) {
          let v = TREE_INT_CST_LOW(rhs);
          state.assignValue(lhs, makeIntAV(v), isn);
        } else {
          let value = expr_literal_int(rhs) == 0 ? av.ZERO : av.NONZERO;
          state.assignValue(lhs, value, isn);
        }
      }
      break;
    case NE_EXPR: {
      
      let [op1, op2] = rhs.operands();
      if (DECL_P(op1) && expr_literal_int(op2) == 0) {
        state.assign(lhs, op1, isn);
      }
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
      break;
    
    case ADDR_EXPR:
    case POINTER_PLUS_EXPR:
    case ARRAY_REF:
    case COMPONENT_REF:
    case INDIRECT_REF:
    case FILTER_EXPR:
    case EXC_PTR_EXPR:
    case CONSTRUCTOR:

    case REAL_CST:
    case STRING_CST:

    case CONVERT_EXPR:
    case TRUTH_NOT_EXPR:
    case TRUTH_XOR_EXPR:
    case BIT_FIELD_REF:
      state.remove(lhs);
      break;
    default:
      if (UNARY_CLASS_P(rhs) || BINARY_CLASS_P(rhs) || COMPARISON_CLASS_P(rhs)) {
        state.remove(lhs);
        break;
      }
      print(TREE_CODE(rhs));
      throw new Error("ni");
    }
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
    state.predicate(lhs, val, arg, blame);
  } else {
    state.assignValue(lhs, av.BOTTOM, blame);
  }
};


OutparamCheck.prototype.processCall = function(dest, expr, blame, state) {
  let args = call_args(expr);
  let callable = callable_arg_function_decl(TREE_OPERAND(expr, 1));
  let psem = this.func_param_semantics(callable);
    
  

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
      
      state.update(function(ss) {
        let [s1, s2] = [ss.copy(), ss]; 
        for each (let [vbl, sem] in updates) {
          s1.assignValue(vbl, sem.val, blame);
          s1.assignValue(dest, av.ZERO, blame);
        }
        s2.assignValue(dest, av.NONZERO, blame);
        return [s1,s2];
      });
    } else {
      
      
      
      
      state.update(function(ss) {
        for each (let [vbl, sem] in updates) {
          if (sem == ps.OUTNOFAIL) {
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


OutparamCheck.prototype.check = function() {
  let state = this.cfg.x_exit_block_ptr.stateOut;
  for (let substate in state.substates.getValues()) {
    this.checkSubstate(substate);
  }
}

OutparamCheck.prototype.checkSubstate = function(ss) {
  let rv = ss.get(this.retvar);
  switch (rv) {
  case av.ZERO:
    this.checkSubstateSuccess(ss);
    break;
  case av.NONZERO:
    this.checkSubstateFailure(ss);
    break;
  default:
    throw new Error("ni " + rv);
  }
}

OutparamCheck.prototype.checkSubstateSuccess = function(ss) {
  for (let i = 0; i < this.psem_list.length; ++i) {
    let [v, psem] = [ this.outparam_list[i], this.psem_list[i] ];
    if (psem == ps.INOUT) continue;
    let val = ss.get(v);
    if (val == av.NOT_WRITTEN) {
      this.logResult('succ', 'not_written', 'error');
      this.warn("outparam not written on NS_SUCCEEDED(return value)",
                v, this.formatBlame('Return at', ss, this.retvar));
    } else if (val == av.MAYBE_WRITTEN) {
      this.logResult('succ', 'maybe_written', 'error');
      this.warn("outparam not written on NS_SUCCEEDED(return value)", v,
                this.formatBlame('Return at', ss, this.retvar),
                this.formatBlame('Possibly written by unannotated outparam in call at', ss, v));
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
      this.warn("outparam written on NS_FAILED(return value)", v,
                this.formatBlame('Return at', ss, this.retvar),
                this.formatBlame('Written at', ss, v));
    } else if (val == av.WROTE_NULL) {
      this.logResult('fail', 'wrote_null', 'warning');
      this.warn("NULL written to outparam on NS_FAILED(return value)", v,
                this.formatBlame('Return at', ss, this.retvar),
                this.formatBlame('Written at', ss, v));
    } else {
      this.logResult('fail', '', 'ok');
    }
  }    
}

OutparamCheck.prototype.warn = function() {
  let tag = arguments[0];
  let v = arguments[1];
  let rest = Array.slice(arguments, 2);

  let label = expr_display(v)
  let lines = [ tag + ': ' + label,
              'Outparam declared at: ' + loc_string(location_of(v)) ];
  lines = lines.concat(rest);
  let msg = lines.join('\n    ');
  warning(msg);
}

OutparamCheck.prototype.formatBlame = function(msg, ss, v) {
  let blame = ss.getBlame(v);
  let loc = blame ? loc_string(location_of(blame)) : '?';
  return(msg + ": " + loc);
}

OutparamCheck.prototype.logResult = function(rv, msg, kind) {
  if (LOG_RESULTS) {
    let s = [ '"' + x + '"' for each (x in [ loc_string(location_of(this.fndecl)), function_decl_name(this.fndecl), rv, msg, kind ]) ].join(', ');
    print(":LR: (" + s + ")");
  }
}



let ps = {
  OUTNOFAIL: { label: 'out-no-fail', val: av.WRITTEN },
  OUT:       { label: 'out',   val: av.WRITTEN },
  INOUT:     { label: 'inout',   val: av.WRITTEN },
  MAYBE:     { label: 'maybe', val: av.MAYBE_WRITTEN},  
  CONST:     { label: 'const' }   
};



OutparamCheck.prototype.func_param_semantics = function(callable) {
  let ftype = TREE_TYPE(callable);
  if (TREE_CODE(ftype) == POINTER_TYPE) ftype = TREE_TYPE(ftype);
  
  let nofail = TREE_TYPE(TREE_TYPE(ftype)) == VOID_TYPE;

  
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
      
      sem = ps.OUTNOFAIL;
    } else {
      if (params) sem = param_semantics(params[i]);
      if (sem == undefined) {
        sem = param_semantics_by_type(types[i]);
        
        if (i < types.length - 1 && sem == ps.OUT) sem = ps.MAYBE;
      }
      if (sem == ps.OUT && nofail) sem = ps.OUTNOFAIL;
    }
    if (ans == undefined) throw new Error("assert");
    ans.push(sem);
  }
  return ans;
}



function param_semantics(decl) {
  for each (let attr in rectify_attributes(DECL_ATTRIBUTES(decl))) {
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


function param_semantics_by_type(type) {
  switch (TREE_CODE(type)) {
  case POINTER_TYPE:
    let pt = TREE_TYPE(type);
    switch (TREE_CODE(pt)) {
    case RECORD_TYPE:
      
      return ps.CONST;
    case POINTER_TYPE:
      
      let ppt = TREE_TYPE(pt);
      let tname = TYPE_NAME(ppt);
      if (tname == undefined) return ps.CONST;
      let name = decl_name_string(tname);
      return name == 'void' || name == 'char' || name == 'PRUnichar' ||
        name.substr(0, 3) == 'nsI' ?
        ps.OUT : ps.CONST;
    case INTEGER_TYPE: {
      let name = decl_name_string(TYPE_NAME(pt));
      return name != 'char' && name != 'PRUnichar' ? ps.OUT : ps.CONST;
    }
    case ENUMERAL_TYPE:
    case REAL_TYPE:
    case UNION_TYPE:
      return TYPE_READONLY(pt) ? ps.CONST : ps.OUT;
    case FUNCTION_TYPE:
    case VOID_TYPE:
      return ps.CONST;
    default:
      print("Y " + TREE_CODE(pt));
      print('Y ' + type_string(pt));
      throw new Error("ni");
    }
    break;
  case REFERENCE_TYPE:
    let rt = TREE_TYPE(type);
    return !TYPE_READONLY(rt) && is_string_type(rt) ? ps.OUT : ps.CONST;
  case INTEGER_TYPE:
  case REAL_TYPE:
  case ENUMERAL_TYPE:
  case RECORD_TYPE:
  case UNION_TYPE:    
    return ps.CONST;
  default:
    print("Z " + TREE_CODE(type));
    print('Z ' + type_string(type));
    throw new Error("ni");
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

