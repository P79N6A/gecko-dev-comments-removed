




































#ifndef __PROFILE_H__
#define __PROFILE_H__

class Profile
{
public:
  Profile();
  ~Profile();

  virtual BOOL getBool(char * key, BOOL * value) = 0;
  virtual BOOL setBool(char * key, BOOL value) = 0;

  virtual BOOL getString(char * key, char * string, int size) = 0;
  virtual BOOL setString(char * key, char * string) = 0;
};

#endif
