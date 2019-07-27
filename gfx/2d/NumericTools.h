




#ifndef MOZILLA_GFX_NUMERICTOOLS_H_
#define MOZILLA_GFX_NUMERICTOOLS_H_




static int32_t
RoundDownToMultiple(int32_t x, int32_t aMultiplier)
{
  
  
  int mod = x % aMultiplier;
  if (x > 0) {
    return x - mod;
  }
  return mod ? x - aMultiplier - mod : x;
}




static int32_t
RoundUpToMultiple(int32_t x, int32_t aMultiplier)
{
  int mod = x % aMultiplier;
  if (x > 0) {
    return mod ? x + aMultiplier - mod : x;
  }
  return x - mod;
}

#endif 
