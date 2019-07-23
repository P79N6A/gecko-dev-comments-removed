





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

function attrs(tree) {
  let a = DECL_P(tree) ? DECL_ATTRIBUTES(tree) : TYPE_ATTRIBUTES(TREE_TYPE(tree));
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




function isRed(x) { return hasUserAttribute(x, RED); }
function isTurnRed(x) { return hasUserAttribute(x, TURN_RED); }

function process_tree(fndecl)
{
  if (!(isRed(fndecl) || isTurnRed(fndecl))) {
    
    
    
    
    
    
    
    let a = new RedGreenCheck(fndecl, 0);
    if (a.hasRed)
      a.run();
  }
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
            loc = location_of(stack[i]);
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

