




































package nu.validator.htmlparser.cpptranslate;

import java.util.List;

import japa.parser.ast.expr.AnnotationExpr;
import japa.parser.ast.expr.MarkerAnnotationExpr;
import japa.parser.ast.type.ReferenceType;
import japa.parser.ast.visitor.VoidVisitorAdapter;

public class AnnotationHelperVisitor<T> extends VoidVisitorAdapter<T> {

    protected List<AnnotationExpr> currentAnnotations;

    protected boolean nsUri() {
        return hasAnnotation("NsUri");
    }

    protected boolean prefix() {
        return hasAnnotation("Prefix");
    }

    protected boolean local() {
        return hasAnnotation("Local");
    }

    protected boolean literal() {
        return hasAnnotation("Literal");
    }

    protected boolean inline() {
        return hasAnnotation("Inline");
    }

    protected boolean noLength() {
        return hasAnnotation("NoLength");
    }

    protected boolean virtual() {
        return hasAnnotation("Virtual");
    }

    private boolean hasAnnotation(String anno) {
        if (currentAnnotations == null) {
            return false;
        }
        for (AnnotationExpr ann : currentAnnotations) {
            if (ann instanceof MarkerAnnotationExpr) {
                MarkerAnnotationExpr marker = (MarkerAnnotationExpr) ann;
                if (marker.getName().getName().equals(anno)) {
                    return true;
                }
            }
        }
        return false;
    }

    protected Type convertType(japa.parser.ast.type.Type type, int modifiers) {
        if (type instanceof ReferenceType) {
            ReferenceType referenceType = (ReferenceType) type;
            return new Type(convertTypeName(referenceType.getType().toString()), referenceType.getArrayCount(), noLength(), modifiers);
        } else {
            return new Type(convertTypeName(type.toString()), 0, false, modifiers);
        }
    }

    private String convertTypeName(String name) {
        if ("String".equals(name)) {
            if (local()) {
                return "@Local";
            }
            if (nsUri()) {
                return "@NsUri";
            }
            if (prefix()) {
                return "@Prefix";
            }
            if (literal()) {
                return "@Literal";
            }
        }
        return name;
    }

}
