



package org.mozilla.gecko.background.healthreport;















public abstract class Environment extends EnvironmentV2 {
  
  
  public static final int CURRENT_VERSION = 3;

  public static enum UIType {
    
    DEFAULT("default"),

    
    LARGE_TABLET("largetablet"),

    
    SMALL_TABLET("smalltablet");

    private final String label;

    private UIType(final String label) {
      this.label = label;
    }

    public String toString() {
      return this.label;
    }

    public static UIType fromLabel(final String label) {
      for (UIType type : UIType.values()) {
        if (type.label.equals(label)) {
          return type;
        }
      }

      throw new IllegalArgumentException("Bad enum value: " + label);
    }
  }

  public UIType uiType = UIType.DEFAULT;

  


  public int uiMode = 0;     

  


  public int screenXInMM;
  public int screenYInMM;

  


  public int screenLayout = 0;  

  public boolean hasHardwareKeyboard;

  public Environment() {
    this(Environment.HashAppender.class);
  }

  public Environment(Class<? extends EnvironmentAppender> appenderClass) {
    super(appenderClass);
    version = CURRENT_VERSION;
  }

  @Override
  protected void appendHash(EnvironmentAppender appender) {
    super.appendHash(appender);

    
    appender.append(hasHardwareKeyboard ? 1 : 0);
    appender.append(uiType.toString());
    appender.append(uiMode);
    appender.append(screenLayout);
    appender.append(screenXInMM);
    appender.append(screenYInMM);
  }
}
