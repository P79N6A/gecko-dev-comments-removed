



package org.mozilla.gecko.background.healthreport;















public abstract class Environment extends EnvironmentV1 {
  
  public static final int CURRENT_VERSION = 2;

  public String osLocale;                
  public String appLocale;
  public int acceptLangSet;
  public String distribution;            

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

    
    appender.append(osLocale);
    appender.append(appLocale);
    appender.append(acceptLangSet);
    appender.append(distribution);
  }
}
