




































public class Test11 extends Test2{

  static {
     System.loadLibrary("ojiapijnitests");
  
  }

  public void test(int i){
     mprint(i);
  }
  
  public native void mprint(int i);

  public static native void mprint_static(int i);
  
  public void jprint(int i){
      System.out.println("i="+i);
  }

  public static void jprint_static(int i){
      System.out.println("i="+i);
  }

  public native int Test1_method3_native(boolean bb, byte by, char ch, short sh, int in, long lg, float fl, double db, String str, String strarr[]);

  public static native int Test1_method3_native_static(boolean bb, byte by, char ch, short sh, int in, long lg, float fl, double db, String str, String strarr[]);

}

