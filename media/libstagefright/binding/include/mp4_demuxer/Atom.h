



#ifndef ATOM_H_
#define ATOM_H_

namespace mp4_demuxer {

class Atom
{
public:
  Atom()
    : mValid(false)
  {
  }
  virtual bool IsValid()
  {
    return mValid;
  }
protected:
  bool mValid;
};

}

#endif 
