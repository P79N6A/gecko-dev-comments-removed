




































package nu.validator.htmlparser.cpptranslate;

import japa.parser.JavaParser;
import japa.parser.ParseException;
import japa.parser.ast.CompilationUnit;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;

public class Main {

    static final String[] H_LIST = {
        "Tokenizer",
        "TreeBuilder",
        "MetaScanner",
        "AttributeName",
        "ElementName",
        "HtmlAttributes",
        "StackNode",
        "UTF16Buffer",
        "StateSnapshot",
        "Portability",
    };
    
    private static final String[] CPP_LIST = {
        "Tokenizer",
        "TreeBuilder",
        "MetaScanner",
        "AttributeName",
        "ElementName",
        "HtmlAttributes",
        "StackNode",
        "UTF16Buffer",
        "StateSnapshot",
    };
    
    




    public static void main(String[] args) throws ParseException, IOException {
        CppTypes cppTypes = new CppTypes(new File(args[2]));
        SymbolTable symbolTable = new SymbolTable();
        
        File javaDirectory = new File(args[0]);
        File targetDirectory = new File(args[1]);
        File cppDirectory = targetDirectory;
        File javaCopyDirectory = new File(targetDirectory, "javasrc");
        
        for (int i = 0; i < H_LIST.length; i++) {
            parseFile(cppTypes, javaDirectory, cppDirectory, H_LIST[i], ".h", new HVisitor(cppTypes, symbolTable));

        }
        for (int i = 0; i < CPP_LIST.length; i++) {
            parseFile(cppTypes, javaDirectory, cppDirectory, CPP_LIST[i], ".cpp", new CppVisitor(cppTypes, symbolTable));
        }
        cppTypes.finished();
    }

    private static void copyFile(File input, File output) throws IOException {
        if (input.getCanonicalFile().equals(output.getCanonicalFile())) {
            return; 
        }
        
        FileInputStream in = new FileInputStream(input);
        FileOutputStream out = new FileOutputStream(output);
        int b;
        while ((b = in.read()) != -1) {
            out.write(b);
        }
        out.flush();
        out.close();
        in.close();
    }
    
    private static void parseFile(CppTypes cppTypes, File javaDirectory, File cppDirectory, String className, String fne, CppVisitor visitor) throws ParseException,
            FileNotFoundException, UnsupportedEncodingException, IOException {
        File file = new File(javaDirectory, className + ".java");
        String license = new LicenseExtractor(file).extract();
        CompilationUnit cu = JavaParser.parse(new NoCppInputStream(new FileInputStream(file)), "utf-8");
        LabelVisitor labelVisitor = new LabelVisitor();
        cu.accept(labelVisitor, null);
        visitor.setLabels(labelVisitor.getLabels());
        cu.accept(visitor, null);
        FileOutputStream out = new FileOutputStream(new File(cppDirectory, cppTypes.classPrefix() + className + fne));
        OutputStreamWriter w = new OutputStreamWriter(out, "utf-8");
        w.write(license);
        w.write("\n\n/*\n * THIS IS A GENERATED FILE. PLEASE DO NOT EDIT.\n * Please edit " + className + ".java instead and regenerate.\n */\n\n");
        w.write(visitor.getSource());
        w.close();
    }

}
