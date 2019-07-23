











include("unstable/lazy_types.js");

var matches = {};

function identity(x) x;

function process_cp_pre_genericize(fndecl)
{
    var c = [];
    function calls(t, stack)
    {
        try {
            t.tree_check(CALL_EXPR);
            var fn = callable_arg_function_decl(CALL_EXPR_FN(t));
            if (fn)
                c.push(decl_name_string(fn));
        }
        catch (e if e.TreeCheckError) { }
    }

    walk_tree(DECL_SAVED_TREE(fndecl), calls);

    for (let [fnreplace, pattern] in patterns)
        if (pattern.map(function(e){ return c.some(function(f) { return e == f; }); }).every(identity))
            if (fnreplace != (n = decl_name_string(fndecl)))
                print(fnreplace +" could probably be used in "+ n);
}
