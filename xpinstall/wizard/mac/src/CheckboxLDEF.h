






































#ifndef _CHECKBOXLDEF_H_
#define _CHECKBOXLDEF_H_


#ifdef __cplusplus
extern "C" {
#endif

pascal void main(SInt16 message,Boolean selected,Rect *cellRect,Cell theCell,
	 			 SInt16 dataOffset,SInt16 dataLen,ListHandle theList);
void  		Draw(Boolean selected,Rect *cellRect,Cell theCell,SInt16 dataLen,
		   		 ListHandle theList);
void 		Highlight(Rect *cellRect, Boolean selected);
Rect		DrawEmptyCheckbox(Rect *cellRect);
void		DrawCheckedCheckbox(Rect *cellRect);

#ifdef __cplusplus
}
#endif


#endif