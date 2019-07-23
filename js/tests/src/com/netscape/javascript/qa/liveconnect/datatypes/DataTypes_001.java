



package com.netscape.javascript.qa.liveconnect.datatypes;

import com.netscape.javascript.qa.liveconnect.*;















public class DataTypes_001 extends LiveConnectTest {
    public DataTypes_001() {
        super();
    }

    public static void main( String[] args ) {
        DataTypes_001 test = new DataTypes_001();
        test.start();
    }

    public void executeTest() {

        doStaticFieldTests( 
            "DT.PUB_STATIC_BOOLEAN",
            "java.lang.Boolean",
            (Object) new Boolean(DataTypeClass.PUB_STATIC_FINAL_BOOLEAN) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_BOOLEAN",
            "java.lang.Boolean",
            (Object) new Boolean(DataTypeClass.PUB_STATIC_FINAL_BOOLEAN) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_BYTE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_BYTE) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_BYTE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_BYTE) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_SHORT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_SHORT) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_SHORT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_SHORT) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_INT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_INT) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_INT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_INT) );
            
        doStaticFieldTests( 
            "DT.PUB_STATIC_LONG",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_LONG) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_LONG",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_LONG) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_FLOAT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_FLOAT) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_FLOAT",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_FLOAT) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_DOUBLE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_DOUBLE) );
            
        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_DOUBLE",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_DOUBLE) );

        doStaticFieldTests(
            "DT.PUB_STATIC_FINAL_CHAR",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_CHAR) );

        doStaticFieldTests(
            "DT.PUB_STATIC_CHAR",
            "java.lang.Double",
            (Object) new Double(DataTypeClass.PUB_STATIC_FINAL_CHAR) );

        doStaticFieldTests( 
            "DT.PUB_STATIC_STRING",
            "java.lang.String",
            (Object) new String(DataTypeClass.PUB_STATIC_FINAL_STRING) );

        doStaticFieldTests(   
            "DT.PUB_STATIC_FINAL_STRING",
            "java.lang.String",
            (Object) new String(DataTypeClass.PUB_STATIC_FINAL_STRING) );
    }

    public void setupTestEnvironment() {
        super.setupTestEnvironment();
        global.eval( "var DT = Packages.com.netscape.javascript.qa.liveconnect.DataTypeClass"  );
        file.bugnumber= "301113";
    }
    
    public void doStaticFieldTests( String field, String className, Object value ) {
        getPublicStaticField( field, className, value );
        
        if ( field.startsWith( "DT.PUB_STATIC_FINAL" )) {
            setPublicStaticFinalField( field, className, value );                
        } else {
            setPublicStaticField( field, className, value );
        }
        
        setJavaScriptVariable( field, className, value );
    }        
    
    




    public void getPublicStaticField( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object actual = null;
        String expect = null;
        String typeof = null;
        String expectedType = null;
        
        
        try {
            global.eval( "var myobject = " +description );            
            actual = global.getMember( "myobject" );
            typeof = (String) global.eval( "typeof myobject" );
            expect  = Class.forName(className).getName();
            
            if ( value instanceof Number ) {
                expectedType = "number";
            } else if ( value instanceof Boolean ) {
                expectedType = "boolean";
            } else {
                expectedType = "object";
            }                
            
        } catch ( ClassNotFoundException e ) {
        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        
        addTestCase( 
            "global.eval( \"var myobject = \" + " + description +");"+
            "global.eval( \"typeof myobject\" ).equals(\""+expectedType+"\");",
            "true",
            typeof.equals(expectedType) +"",
            exception );
        
        addTestCase( "( " + description +" ).getClass()",
            expect,
            actual.getClass().getName(),
            exception );
            
        addTestCase( "( " +description + " == " + value.toString() +" )",
            "true",
            actual.equals( value ) + "",
            exception );

        addTestCase( "\"" +actual.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            actual.toString().equals(value.toString()) +"",
            exception );
    }

    



    
    public void setPublicStaticField( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object before = null;
        Object after = null;
        String expect = null;
       
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("New Value!") 
                            ;        
                            
        try {
            before = global.eval( description );

            
            if ( className.equals("java.lang.String") ){
                global.eval( description +" = \"" + newValue.toString() +"\"" );
            } else {
                global.eval( description +" = " + newValue );                
            }        
    
            after = global.eval( description );
            expect = Class.forName( className ).getName();

        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        addTestCase( "global.eval(\""+description+" = "+newValue.toString()+
            "\"); after = global.eval( \"" + description +"\" );" +
            "after.getClass().getName()",
            expect,
            after.getClass().getName(),
            exception );
            
        addTestCase( "( "+after.toString() +" ).equals(" + newValue.toString() +")",
            "true",
            after.equals( newValue ) + "",
            exception );

        addTestCase( "\"" +after.toString() + "\".equals(\"" + newValue.toString() +"\")",
            "true",
            after.toString().equals(newValue.toString()) +"",
            exception );        
    }       
    
    




 
    public void setPublicStaticFinalField ( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object before = null;
        Object after = null;
        String expect = null;
        
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("\"New Value!\"") 
                            ;        
                            
        try {
            expect = Class.forName( className ).getName();
        } catch ( Exception e ) {
            exception = e.toString();
        }            
            before = global.eval( description );
            global.eval( description +" = " + newValue.toString() );
            after = global.eval( description );
        
        
        
        addTestCase( "( " + description +" ).getClass()",
            expect,
            after.getClass().getName(),
            exception );
            
        
            
        addTestCase( "( " +description + " == " + value.toString() +" )",
            "true",
            after.equals( value ) + "",
            exception );
            
        
        

        addTestCase( "\"" +after.toString() + "\".equals(\"" + value.toString() +"\")",
            "true",
            after.toString().equals(value.toString()) +"",
            exception );        
            
        
        
        
        addTestCase( "( " + before +".equals(" + after +") ) ",
                    "true",
                    ( before.equals(after) ) +"",
                    exception );

    }           

    public void setJavaScriptVariable( String field, String className, Object value ) {
        String description = field;
        String exception = null;
        Object actual = null;
        String expect = null;
        
        Object newValue =   className.equals("java.lang.Double") 
                            ? new Double(0)  
                            : className.equals("java.lang.Boolean")
                              ? new Boolean(false)
                              : (Object) new String("New Value!") 
                            ;        
        try {
            global.eval( "var myobject = " + description );
            global.setMember( "myobject", newValue );
            actual = global.getMember( "myobject" );
            expect = Class.forName( className ).getName();
        } catch ( Exception e ) {
            exception = e.toString();
        }            
        
        addTestCase( "global.eval(\"var myobject = " + description +"\" ); " +
            "global.setMember( \"myobject\", " + newValue.toString() +");" +
            "actual = global.getMember( \"myobject\" ); "+
            "actual.getClass().getName()",
            expect,
            actual.getClass().getName(),
            exception );
            
        addTestCase( "( " + description + " == " + newValue.toString() +" )",
            "true",
            actual.equals( newValue ) + "",
            exception );

        addTestCase( "\"" +actual.toString() + "\".equals(\"" + newValue.toString() +"\")",
            "true",
            actual.toString().equals(newValue.toString()) +"",
            exception );        
    }           
 }