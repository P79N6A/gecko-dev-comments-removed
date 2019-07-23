
















#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../internal.h"







static int oc_tagcompare(const char *_s1,const char *_s2,int _n){
  int c;
  for(c=0;c<_n;c++){
    if(toupper(_s1[c])!=toupper(_s2[c]))return !0;
  }
  return _s1[c]!='=';
}



void th_info_init(th_info *_info){
  memset(_info,0,sizeof(*_info));
  _info->version_major=TH_VERSION_MAJOR;
  _info->version_minor=TH_VERSION_MINOR;
  _info->version_subminor=TH_VERSION_SUB;
  _info->keyframe_granule_shift=6;
}

void th_info_clear(th_info *_info){
  memset(_info,0,sizeof(*_info));
}



void th_comment_init(th_comment *_tc){
  memset(_tc,0,sizeof(*_tc));
}

void th_comment_add(th_comment *_tc,char *_comment){
  int comment_len;
  _tc->user_comments=_ogg_realloc(_tc->user_comments,
   (_tc->comments+2)*sizeof(*_tc->user_comments));
  _tc->comment_lengths=_ogg_realloc(_tc->comment_lengths,
   (_tc->comments+2)*sizeof(*_tc->comment_lengths));
  comment_len=strlen(_comment);
  _tc->comment_lengths[_tc->comments]=comment_len;
  _tc->user_comments[_tc->comments]=_ogg_malloc(comment_len+1);
  memcpy(_tc->user_comments[_tc->comments],_comment,comment_len+1);
  _tc->comments++;
  _tc->user_comments[_tc->comments]=NULL;
}

void th_comment_add_tag(th_comment *_tc,char *_tag,char *_val){
  char *comment;
  int   tag_len;
  int   val_len;
  tag_len=strlen(_tag);
  val_len=strlen(_val);
  
  comment=_ogg_malloc(tag_len+val_len+2);
  memcpy(comment,_tag,tag_len);
  comment[tag_len]='=';
  memcpy(comment+tag_len+1,_val,val_len+1);
  th_comment_add(_tc,comment);
  _ogg_free(comment);
}

char *th_comment_query(th_comment *_tc,char *_tag,int _count){
  long i;
  int  found;
  int  tag_len;
  tag_len=strlen(_tag);
  found=0;
  for(i=0;i<_tc->comments;i++){
    if(!oc_tagcompare(_tc->user_comments[i],_tag,tag_len)){
      
      if(_count==found++)return _tc->user_comments[i]+tag_len+1;
    }
  }
  
  return NULL;
}

int th_comment_query_count(th_comment *_tc,char *_tag){
  long i;
  int  tag_len;
  int  count;
  tag_len=strlen(_tag);
  count=0;
  for(i=0;i<_tc->comments;i++){
    if(!oc_tagcompare(_tc->user_comments[i],_tag,tag_len))count++;
  }
  return count;
}

void th_comment_clear(th_comment *_tc){
  if(_tc!=NULL){
    long i;
    for(i=0;i<_tc->comments;i++)_ogg_free(_tc->user_comments[i]);
    _ogg_free(_tc->user_comments);
    _ogg_free(_tc->comment_lengths);
    _ogg_free(_tc->vendor);
    memset(_tc,0,sizeof(*_tc));
  }
}
