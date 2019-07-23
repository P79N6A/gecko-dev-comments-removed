




































package nu.validator.htmlparser.cpptranslate;

import japa.parser.ast.body.ClassOrInterfaceDeclaration;
import japa.parser.ast.body.FieldDeclaration;
import japa.parser.ast.body.MethodDeclaration;

public class SymbolTableVisitor extends AnnotationHelperVisitor<SymbolTable> {

    private String javaClassName;
    
    


    @Override public void visit(FieldDeclaration n, SymbolTable arg) {
        currentAnnotations = n.getAnnotations();
        arg.putFieldType(javaClassName, n.getVariables().get(0).getId().getName(), convertType(n.getType(), n.getModifiers()));
    }

    


    @Override public void visit(MethodDeclaration n, SymbolTable arg) {
        currentAnnotations = n.getAnnotations();
        arg.putMethodReturnType(javaClassName, n.getName(), convertType(n.getType(), n.getModifiers()));
    }

    


    @Override public void visit(ClassOrInterfaceDeclaration n, SymbolTable arg) {
        javaClassName = n.getName();
    }

}
