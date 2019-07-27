








































#ifndef __PS_LATTICE_INTERNAL_H__
#define __PS_LATTICE_INTERNAL_H__









typedef struct latlink_list_s {
    ps_latlink_t *link;
    struct latlink_list_s *next;
} latlink_list_t;




struct ps_lattice_s {
    int refcount;      

    logmath_t *lmath;    
    ps_search_t *search; 
    dict_t *dict;	 
    int32 silence;       
    int32 frate;         

    ps_latnode_t *nodes;  
    ps_latnode_t *start;  
    ps_latnode_t *end;    

    frame_idx_t n_frames;    
    int16 n_nodes;     
    int32 final_node_ascr; 
    int32 norm;        
    char *hyp_str;     

    listelem_alloc_t *latnode_alloc;     
    listelem_alloc_t *latlink_alloc;     
    listelem_alloc_t *latlink_list_alloc; 

    
    latlink_list_t *q_head; 
    latlink_list_t *q_tail; 
};








struct ps_latlink_s {
    struct ps_latnode_s *from;	
    struct ps_latnode_s *to;	
    struct ps_latlink_s *best_prev;
    int32 ascr;			
    int32 path_scr;		
    frame_idx_t ef;			
    int32 alpha;                
    int32 beta;                 
};







struct ps_latnode_s {
    int32 id;			
    int32 wid;			
    int32 basewid;		
    
    int32 fef;			
    int32 lef;			
    frame_idx_t sf;			
    int16 reachable;		
    int32 node_id;		
    union {
        glist_t velist;         
	int32 fanin;		
	int32 rem_score;	
	int32 best_exit;	
    } info;
    latlink_list_t *exits;      
    latlink_list_t *entries;    

    struct ps_latnode_s *alt;   
    struct ps_latnode_s *next;	
};




typedef struct dag_seg_s {
    ps_seg_t base;       
    ps_latlink_t **links;   
    int32 norm;     
    int16 n_links;  
    int16 cur;      
} dag_seg_t;







typedef struct ps_latpath_s {
    ps_latnode_t *node;            
    struct ps_latpath_s *parent;   
    struct ps_latpath_s *next;     
    int32 score;                  
} ps_latpath_t;




typedef struct ps_astar_s {
    ps_lattice_t *dag;
    ngram_model_t *lmset;
    float32 lwf;

    frame_idx_t sf;
    frame_idx_t ef;
    int32 w1;
    int32 w2;

    int32 n_hyp_tried;
    int32 n_hyp_insert;
    int32 n_hyp_reject;
    int32 insert_depth;
    int32 n_path;

    ps_latpath_t *path_list;
    ps_latpath_t *path_tail;
    ps_latpath_t *top;

    glist_t hyps;	             
    listelem_alloc_t *latpath_alloc; 
} ps_astar_t;




typedef struct astar_seg_s {
    ps_seg_t base;
    ps_latnode_t **nodes;
    int n_nodes;
    int cur;
} astar_seg_t;




ps_lattice_t *ps_lattice_init_search(ps_search_t *search, int n_frame);




void ps_lattice_penalize_fillers(ps_lattice_t *dag, int32 silpen, int32 fillpen);




void ps_lattice_delete_unreachable(ps_lattice_t *dag);




void ps_lattice_pushq(ps_lattice_t *dag, ps_latlink_t *link);




ps_latlink_t *ps_lattice_popq(ps_lattice_t *dag);




void ps_lattice_delq(ps_lattice_t *dag);




latlink_list_t *latlink_list_new(ps_lattice_t *dag, ps_latlink_t *link,
                                 latlink_list_t *next);




char const *ps_lattice_hyp(ps_lattice_t *dag, ps_latlink_t *link);




ps_seg_t *ps_lattice_seg_iter(ps_lattice_t *dag, ps_latlink_t *link,
                              float32 lwf);










ps_astar_t *ps_astar_start(ps_lattice_t *dag,
                           ngram_model_t *lmset,
                           float32 lwf,
                           int sf, int ef,
                           int w1, int w2);






ps_latpath_t *ps_astar_next(ps_astar_t *nbest);




void ps_astar_finish(ps_astar_t *nbest);




char const *ps_astar_hyp(ps_astar_t *nbest, ps_latpath_t *path);




ps_seg_t *ps_astar_seg_iter(ps_astar_t *astar, ps_latpath_t *path, float32 lwf);


#endif 
