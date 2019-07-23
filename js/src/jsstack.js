





require({ after_gcc_pass: 'cfg' });
include('gcc_util.js');
include('unstable/adts.js');
include('unstable/analysis.js');
include('unstable/lazy_types.js');
include('unstable/esp.js');

var Zero_NonZero = {};
include('unstable/zero_nonzero.js', Zero_NonZero);


MapFactory.use_injective = true;

















const RED = 'JS_REQUIRES_STACK';
const TURN_RED = 'JS_FORCES_STACK';
const IGNORE_ERRORS = 'JS_IGNORE_STACK';

function attrs(tree) {
  let a = DECL_P(tree) ? DECL_ATTRIBUTES(tree) : TYPE_ATTRIBUTES(tree);
  return translate_attributes(a);
}

function hasUserAttribute(tree, attrname) {
  let attributes = attrs(tree);
  if (attributes) {
    for (let i = 0; i < attributes.length; i++) {
      let attr = attributes[i];
      if (attr.name == 'user' && attr.value.length == 1 && attr.value[0] == attrname)
        return true;
    }
  }
  return false;
}

function process_tree_type(d)
{
  let t = dehydra_convert(d);
  if (t.isFunction)
    return;

  if (t.typedef !== undefined)
    if (isRed(TYPE_NAME(d)))
      error("Typedef declaration is annotated JS_REQUIRES_STACK: the annotation should be on the type itself", t.loc);
  
  if (hasAttribute(t, RED)) {
    error("Non-function is annotated JS_REQUIRES_STACK", t.loc);
    return;
  }
  
  for (let st = t; st !== undefined && st.isPointer; st = st.type) {
    if (hasAttribute(st, RED)) {
      error("Non-function is annotated JS_REQUIRES_STACK", t.loc);
      return;
    }
    
    if (st.parameters)
      return;
  }
}

function process_tree_decl(d)
{
  
  if (TREE_CODE(d) != VAR_DECL)
    return;
  
  let i = DECL_INITIAL(d);
  if (!i)
    return;
  
  assignCheck(i, TREE_TYPE(d), function() { return location_of(d); });

  functionPointerWalk(i, d);
}




function isRed(x) { return hasUserAttribute(x, RED); }
function isTurnRed(x) { return hasUserAttribute(x, TURN_RED); }

function process_tree(fndecl)
{
  if (hasUserAttribute(fndecl, IGNORE_ERRORS))
    return;

  if (!(isRed(fndecl) || isTurnRed(fndecl))) {
    
    
    
    
    
    
    
    let a = new RedGreenCheck(fndecl, 0);
    if (a.hasRed)
      a.run();
  }
  
  functionPointerCheck(fndecl);
}

function RedGreenCheck(fndecl, trace) {
  
  this._fndecl = fndecl;

  
  
  
  
  
  this._state_var_decl = fndecl;
  let state_var = new ESP.PropVarSpec(this._state_var_decl, true, undefined);

  
  let cfg = function_decl_cfg(fndecl);
  ESP.Analysis.apply(this, [cfg, [state_var], Zero_NonZero.meet, trace]);
  this.join = Zero_NonZero.join;

  
  
  
  
  
  
  this.hasRed = false;
  let self = this;         
  for (let bb in cfg_bb_iterator(cfg)) {
    for (let isn in bb_isn_iterator(bb)) {
      
      
      
      isn.redInfo = undefined;
      walk_tree(isn, function(t, stack) {
        function getLocation(skiptop) {
          if (!skiptop) {
            let loc = location_of(t);
            if (loc !== undefined)
              return loc;
          }
          
          for (let i = stack.length - 1; i >= 0; --i) {
            let loc = location_of(stack[i]);
            if (loc !== undefined)
              return loc;
          }
          return location_of(DECL_SAVED_TREE(fndecl));
        }
                  
        switch (TREE_CODE(t)) {
          case FIELD_DECL:
            if (isRed(t)) {
              let varName = dehydra_convert(t).name;
              
              isn.redInfo = ["cannot access JS_REQUIRES_STACK variable " + varName,
                             getLocation(true)];
              self.hasRed = true;
            }
            break;
          case CALL_EXPR:
          {
            let callee = call_function_decl(t);
            if (callee) {
              if (isRed(callee)) {
                let calleeName = dehydra_convert(callee).name;
                isn.redInfo = ["cannot call JS_REQUIRES_STACK function " + calleeName,
                              getLocation(false)];
                self.hasRed = true;
              } else if (isTurnRed(callee)) {
                isn.turnRed = true;
              }
            }
            else {
              let fntype = TREE_CHECK(
                TREE_TYPE( 
                  TREE_TYPE( 
                    CALL_EXPR_FN(t)
                  )
                ),
                FUNCTION_TYPE, METHOD_TYPE);
              if (isRed(fntype)) {
                isn.redInfo = ["cannot call JS_REQUIRES_STACK function pointer",
                               getLocation(false)];
                self.hasRed = true;
              }
              else if (isTurnRed(fntype)) {
                isn.turnRed = true;
              }
            }
          }
          break;
        }
      });
    }
  }

  
  this._zeroNonzero = new Zero_NonZero.Zero_NonZero();
}

RedGreenCheck.prototype = new ESP.Analysis;

RedGreenCheck.prototype.flowStateCond = function(isn, truth, state) {
  
  this._zeroNonzero.flowStateCond(isn, truth, state);
};

RedGreenCheck.prototype.flowState = function(isn, state) {
  
  
    this._zeroNonzero.flowState(isn, state);
  
  
  
  
  
  let stackState = state.get(this._state_var_decl);
  let green = stackState != 1 && stackState != ESP.NOT_REACHED;
  let redInfo = isn.redInfo;
  if (green && redInfo) {
    error(redInfo[0], redInfo[1]);
    isn.redInfo = undefined;  
  }

  
  
  if (isn.turnRed)
    state.assignValue(this._state_var_decl, 1, isn);
};

function followTypedefs(type)
{
  while (type.typedef !== undefined)
    type = type.typedef;
  return type;
}

function assignCheck(source, destType, locfunc)
{
  if (TREE_CODE(destType) != POINTER_TYPE)
    return;
    
  let destCode = TREE_CODE(TREE_TYPE(destType));
  if (destCode != FUNCTION_TYPE && destCode != METHOD_TYPE)
    return;
  
  if (isRed(TREE_TYPE(destType)))
    return;

  while (TREE_CODE(source) == NOP_EXPR)
    source = source.operands()[0];
  
  

  if (TREE_CODE(source) == ADDR_EXPR) {
    let sourcefn = source.operands()[0];
    
    
    
    if (TREE_CODE(sourcefn) != FUNCTION_DECL)
      return;
    
    if (isRed(sourcefn))
      error("Assigning non-JS_REQUIRES_STACK function pointer from JS_REQUIRES_STACK function " + dehydra_convert(sourcefn).name, locfunc());
  }
  else {
    let sourceType = TREE_TYPE(TREE_TYPE(source).tree_check(POINTER_TYPE));
    switch (TREE_CODE(sourceType)) {
      case FUNCTION_TYPE:
      case METHOD_TYPE:
        if (isRed(sourceType))
          error("Assigning non-JS_REQUIRES_STACK function pointer from JS_REQUIRES_STACK function pointer", locfunc());
        break;
    }
  }
}






function functionPointerWalk(t, baseloc)
{
  walk_tree(t, function(t, stack) {
    function getLocation(skiptop) {
      if (!skiptop) {
        let loc = location_of(t);
        if (loc !== undefined)
          return loc;
      }
          
      for (let i = stack.length - 1; i >= 0; --i) {
        let loc = location_of(stack[i]);
        if (loc !== undefined)
          return loc;
      }
      return location_of(baseloc);
    }
                  
    switch (TREE_CODE(t)) {
      case GIMPLE_MODIFY_STMT: {
        let [dest, source] = t.operands();
        assignCheck(source, TREE_TYPE(dest), getLocation);
        break;
      }
      case CONSTRUCTOR: {
        let ttype = TREE_TYPE(t);
        switch (TREE_CODE(ttype)) {
          case RECORD_TYPE:
          case UNION_TYPE: {
            for each (let ce in VEC_iterate(CONSTRUCTOR_ELTS(t)))
              assignCheck(ce.value, TREE_TYPE(ce.index), getLocation);
            break;
          }
          case ARRAY_TYPE: {
            let eltype = TREE_TYPE(ttype);
            for each (let ce in VEC_iterate(CONSTRUCTOR_ELTS(t)))
              assignCheck(ce.value, eltype, getLocation);
            break;
          }
          default:
            warning("Unexpected type in initializer: " + TREE_CODE(TREE_TYPE(t)), getLocation());
        }
        break;
      }
      case CALL_EXPR: {
        
        
        let ops = t.operands();
        let funcType = TREE_TYPE( 
          TREE_TYPE(ops[1])); 
        let argTypes = [t for (t in function_type_args(funcType))];
        for (let i = argTypes.length - 1; i >= 0; --i) {
          let destType = argTypes[i];
          let source = ops[i + 3];
          assignCheck(source, destType, getLocation);
        }
        break;
      }
    }
  });
}
  
function functionPointerCheck(fndecl)
{
  let cfg = function_decl_cfg(fndecl);
  for (let bb in cfg_bb_iterator(cfg))
    for (let isn in bb_isn_iterator(bb))
      functionPointerWalk(isn, DECL_SAVED_TREE(fndecl));
}
