




































package nu.validator.htmlparser.cpptranslate;

import japa.parser.ast.stmt.BreakStmt;
import japa.parser.ast.stmt.ContinueStmt;
import japa.parser.ast.visitor.VoidVisitorAdapter;

import java.util.HashSet;
import java.util.Set;

public class LabelVisitor extends VoidVisitorAdapter<Object> {

    private final Set<String> labels = new HashSet<String>();
    
    public LabelVisitor() {
    }

    


    @Override
    public void visit(BreakStmt n, Object arg) {
        String label = n.getId();
        if (label != null) {
            labels.add(label + "_end");
        }
    }

    


    @Override
    public void visit(ContinueStmt n, Object arg) {
        String label = n.getId();
        if (label != null) {
            labels.add(label);
        }
    }

    




    public Set<String> getLabels() {
        return labels;
    }
}
