




































package nu.validator.htmlparser.cpptranslate;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class CppTypes {

    private static Set<String> reservedWords = new HashSet<String>();

    static {
        reservedWords.add("small");
        reservedWords.add("for");
        reservedWords.add("false");
        reservedWords.add("true");
        reservedWords.add("default");
        reservedWords.add("class");
        reservedWords.add("switch");
        reservedWords.add("union");
        reservedWords.add("template");
        reservedWords.add("int");
        reservedWords.add("char");
        reservedWords.add("operator");
        reservedWords.add("or");
        reservedWords.add("and");
        reservedWords.add("not");
        reservedWords.add("xor");
        reservedWords.add("unicode");
    }

    private static final String[] TREE_BUILDER_INCLUDES = { "prtypes",
            "nsIAtom", "nsITimer", "nsString", "nsINameSpaceManager", "nsIContent",
            "nsIDocument", "nsTraceRefcnt", "jArray", "nsHtml5DocumentMode",
            "nsHtml5ArrayCopy", "nsHtml5NamedCharacters", "nsHtml5Parser",
            "nsHtml5Atoms", "nsHtml5ByteReadable", "nsHtml5TreeOperation",
            "nsHtml5PendingNotification", "nsHtml5StateSnapshot", "nsHtml5StackNode", 
            "nsHtml5TreeOpExecutor", "nsHtml5StreamParser" };

    private static final String[] INCLUDES = { "prtypes", "nsIAtom",
            "nsString", "nsINameSpaceManager", "nsIContent", "nsIDocument",
            "nsTraceRefcnt", "jArray", "nsHtml5DocumentMode",
            "nsHtml5ArrayCopy", "nsHtml5NamedCharacters",
            "nsHtml5Atoms", "nsHtml5ByteReadable", "nsIUnicodeDecoder", };

    private static final String[] OTHER_DECLATIONS = {};

    private static final String[] TREE_BUILDER_OTHER_DECLATIONS = { };

    private static final String[] NAMED_CHARACTERS_INCLUDES = { "prtypes",
            "jArray", "nscore" };

    private static final String[] FORWARD_DECLARATIONS = { "nsHtml5StreamParser", };
    
    private static final String[] CLASSES_THAT_NEED_SUPPLEMENT = {
        "MetaScanner",
        "StackNode",
        "TreeBuilder",
        "UTF16Buffer",
    };
    
    private final Map<String, String> atomMap = new HashMap<String, String>();

    private final Writer atomWriter;

    public CppTypes(File atomList) {
        if (atomList == null) {
            atomWriter = null;
        } else {
            try {
                atomWriter = new OutputStreamWriter(new FileOutputStream(
                        atomList), "utf-8");
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public void finished() {
        try {
            if (atomWriter != null) {
                atomWriter.flush();
                atomWriter.close();
            }
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public String classPrefix() {
        return "nsHtml5";
    }

    public String booleanType() {
        return "PRBool";
    }

    public String charType() {
        return "PRUnichar";
    }

    public String intType() {
        return "PRInt32";
    }

    public String stringType() {
        return "nsString*";
    }

    public String localType() {
        return "nsIAtom*";
    }

    public String prefixType() {
        return "nsIAtom*";
    }

    public String nsUriType() {
        return "PRInt32";
    }

    public String falseLiteral() {
        return "PR_FALSE";
    }

    public String trueLiteral() {
        return "PR_TRUE";
    }

    public String nullLiteral() {
        return "nsnull";
    }

    public String encodingDeclarationHandlerType() {
        return "nsHtml5StreamParser*";
    }

    public String nodeType() {
        return "nsIContent*";
    }

    public String xhtmlNamespaceLiteral() {
        return "kNameSpaceID_XHTML";
    }

    public String svgNamespaceLiteral() {
        return "kNameSpaceID_SVG";
    }

    public String xmlnsNamespaceLiteral() {
        return "kNameSpaceID_XMLNS";
    }

    public String xmlNamespaceLiteral() {
        return "kNameSpaceID_XML";
    }

    public String noNamespaceLiteral() {
        return "kNameSpaceID_None";
    }

    public String xlinkNamespaceLiteral() {
        return "kNameSpaceID_XLink";
    }

    public String mathmlNamespaceLiteral() {
        return "kNameSpaceID_MathML";
    }

    public String arrayTemplate() {
        return "jArray";
    }

    public String localForLiteral(String literal) {
        String atom = atomMap.get(literal);
        if (atom == null) {
            atom = createAtomName(literal);
            atomMap.put(literal, atom);
            if (atomWriter != null) {
                try {
                    atomWriter.write("HTML5_ATOM(" + atom + ", \"" + literal
                            + "\")\n");
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        }
        return "nsHtml5Atoms::" + atom;
    }

    private String createAtomName(String literal) {
        String candidate = literal.replaceAll("[^a-zA-Z0-9_]", "_");
        if ("".equals(candidate)) {
            candidate = "emptystring";
        }
        while (atomMap.values().contains(candidate)
                || reservedWords.contains(candidate)) {
            candidate = candidate + '_';
        }
        return candidate;
    }

    public String stringForLiteral(String literal) {
        return '"' + literal + '"';
    }

    public String staticArrayMacro() {
        return "J_ARRAY_STATIC";
    }

    public String[] boilerplateIncludes(String javaClass) {
        if ("TreeBuilder".equals(javaClass)) {
            return TREE_BUILDER_INCLUDES;
        } else {
            return INCLUDES;
        }
    }
    
    public String[] boilerplateDeclarations(String javaClass) {
        if ("TreeBuilder".equals(javaClass)) {
            return TREE_BUILDER_OTHER_DECLATIONS;
        } else {
            return OTHER_DECLATIONS;
        }
    }

    public String[] namedCharactersIncludes() {
        return NAMED_CHARACTERS_INCLUDES;
    }

    public String[] boilerplateForwardDeclarations() {
        return FORWARD_DECLARATIONS;
    }

    public String documentModeHandlerType() {
        return "nsHtml5TreeBuilder*";
    }

    public String documentModeType() {
        return "nsHtml5DocumentMode";
    }

    public String arrayCopy() {
        return "nsHtml5ArrayCopy::arraycopy";
    }

    public String maxInteger() {
        return "PR_INT32_MAX";
    }

    public String constructorBoilerplate(String className) {
        return "MOZ_COUNT_CTOR(" + className + ");";
    }

    public String destructorBoilderplate(String className) {
        return "MOZ_COUNT_DTOR(" + className + ");";
    }

    public String literalType() {
        return "const char*";
    }
    
    public boolean hasSupplement(String javaClass) {
        return Arrays.binarySearch(CLASSES_THAT_NEED_SUPPLEMENT, javaClass) > -1;
    }

}
