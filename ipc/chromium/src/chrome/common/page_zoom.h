



#ifndef CHROME_COMMON_PAGE_ZOOM_H_
#define CHROME_COMMON_PAGE_ZOOM_H_



class PageZoom {
 public:
  enum Function {
    SMALLER  = -1,
    STANDARD = 0,
    LARGER   = 1,
  };

 private:
  PageZoom() {}  
};

#endif
