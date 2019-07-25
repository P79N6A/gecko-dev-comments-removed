




function MayReturnAnalysis() {
  BackwardAnalysis.apply(this, arguments);
  
  this.vbls = create_decl_set();
  
  this.retvar = undefined;
}

MayReturnAnalysis.prototype = new BackwardAnalysis;

MayReturnAnalysis.prototype.flowState = function(isn, state) {
  if (TREE_CODE(isn) == GIMPLE_RETURN) {
    let v = return_expr(isn);
    if (!v)
      return;
    if (v.tree_code() == RESULT_DECL) 
      throw new Error("Weird case hit");
    this.vbls.add(v);
    state.add(v);
    this.retvar = v;
  } else if (TREE_CODE(isn) == GIMPLE_ASSIGN) {
    let lhs = gimple_op(isn, 0);
    let rhs = gimple_op(isn, 1);
    if (DECL_P(rhs) && DECL_P(lhs) && state.has(lhs)) {
      this.vbls.add(rhs);
      state.add(rhs);
    }

    for (let e in isn_defs(isn, 'strong')) {
      if (DECL_P(e)) {
        state.remove(e);
      }
    }
  }
};
