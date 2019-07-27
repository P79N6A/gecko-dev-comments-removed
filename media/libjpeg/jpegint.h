

















typedef enum {            
  JBUF_PASS_THRU,         
  
  JBUF_SAVE_SOURCE,       
  JBUF_CRANK_DEST,        
  JBUF_SAVE_AND_PASS      
} J_BUF_MODE;


#define CSTATE_START    100     /* after create_compress */
#define CSTATE_SCANNING 101     /* start_compress done, write_scanlines OK */
#define CSTATE_RAW_OK   102     /* start_compress done, write_raw_data OK */
#define CSTATE_WRCOEFS  103     /* jpeg_write_coefficients done */
#define DSTATE_START    200     /* after create_decompress */
#define DSTATE_INHEADER 201     /* reading header markers, no SOS yet */
#define DSTATE_READY    202     /* found SOS, ready for start_decompress */
#define DSTATE_PRELOAD  203     /* reading multiscan file in start_decompress*/
#define DSTATE_PRESCAN  204     /* performing dummy pass for 2-pass quant */
#define DSTATE_SCANNING 205     /* start_decompress done, read_scanlines OK */
#define DSTATE_RAW_OK   206     /* start_decompress done, read_raw_data OK */
#define DSTATE_BUFIMAGE 207     /* expecting jpeg_start_output */
#define DSTATE_BUFPOST  208     /* looking for SOS/EOI in jpeg_finish_output */
#define DSTATE_RDCOEFS  209     /* reading file in jpeg_read_coefficients */
#define DSTATE_STOPPING 210     /* looking for EOI in jpeg_finish_decompress */





struct jpeg_comp_master {
  void (*prepare_for_pass) (j_compress_ptr cinfo);
  void (*pass_startup) (j_compress_ptr cinfo);
  void (*finish_pass) (j_compress_ptr cinfo);

  
  boolean call_pass_startup;    
  boolean is_last_pass;         
};


struct jpeg_c_main_controller {
  void (*start_pass) (j_compress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*process_data) (j_compress_ptr cinfo, JSAMPARRAY input_buf,
                        JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail);
};


struct jpeg_c_prep_controller {
  void (*start_pass) (j_compress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*pre_process_data) (j_compress_ptr cinfo, JSAMPARRAY input_buf,
                            JDIMENSION *in_row_ctr, JDIMENSION in_rows_avail,
                            JSAMPIMAGE output_buf,
                            JDIMENSION *out_row_group_ctr,
                            JDIMENSION out_row_groups_avail);
};


struct jpeg_c_coef_controller {
  void (*start_pass) (j_compress_ptr cinfo, J_BUF_MODE pass_mode);
  boolean (*compress_data) (j_compress_ptr cinfo, JSAMPIMAGE input_buf);
};


struct jpeg_color_converter {
  void (*start_pass) (j_compress_ptr cinfo);
  void (*color_convert) (j_compress_ptr cinfo, JSAMPARRAY input_buf,
                         JSAMPIMAGE output_buf, JDIMENSION output_row,
                         int num_rows);
};


struct jpeg_downsampler {
  void (*start_pass) (j_compress_ptr cinfo);
  void (*downsample) (j_compress_ptr cinfo, JSAMPIMAGE input_buf,
                      JDIMENSION in_row_index, JSAMPIMAGE output_buf,
                      JDIMENSION out_row_group_index);

  boolean need_context_rows;    
};


struct jpeg_forward_dct {
  void (*start_pass) (j_compress_ptr cinfo);
  
  void (*forward_DCT) (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
                       JDIMENSION start_row, JDIMENSION start_col,
                       JDIMENSION num_blocks);
};


struct jpeg_entropy_encoder {
  void (*start_pass) (j_compress_ptr cinfo, boolean gather_statistics);
  boolean (*encode_mcu) (j_compress_ptr cinfo, JBLOCKROW *MCU_data);
  void (*finish_pass) (j_compress_ptr cinfo);
};


struct jpeg_marker_writer {
  void (*write_file_header) (j_compress_ptr cinfo);
  void (*write_frame_header) (j_compress_ptr cinfo);
  void (*write_scan_header) (j_compress_ptr cinfo);
  void (*write_file_trailer) (j_compress_ptr cinfo);
  void (*write_tables_only) (j_compress_ptr cinfo);
  
  
  void (*write_marker_header) (j_compress_ptr cinfo, int marker,
                               unsigned int datalen);
  void (*write_marker_byte) (j_compress_ptr cinfo, int val);
};





struct jpeg_decomp_master {
  void (*prepare_for_output_pass) (j_decompress_ptr cinfo);
  void (*finish_output_pass) (j_decompress_ptr cinfo);

  
  boolean is_dummy_pass;        
};


struct jpeg_input_controller {
  int (*consume_input) (j_decompress_ptr cinfo);
  void (*reset_input_controller) (j_decompress_ptr cinfo);
  void (*start_input_pass) (j_decompress_ptr cinfo);
  void (*finish_input_pass) (j_decompress_ptr cinfo);

  
  boolean has_multiple_scans;   
  boolean eoi_reached;          
};


struct jpeg_d_main_controller {
  void (*start_pass) (j_decompress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*process_data) (j_decompress_ptr cinfo, JSAMPARRAY output_buf,
                        JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail);
};


struct jpeg_d_coef_controller {
  void (*start_input_pass) (j_decompress_ptr cinfo);
  int (*consume_data) (j_decompress_ptr cinfo);
  void (*start_output_pass) (j_decompress_ptr cinfo);
  int (*decompress_data) (j_decompress_ptr cinfo, JSAMPIMAGE output_buf);
  
  jvirt_barray_ptr *coef_arrays;
};


struct jpeg_d_post_controller {
  void (*start_pass) (j_decompress_ptr cinfo, J_BUF_MODE pass_mode);
  void (*post_process_data) (j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                             JDIMENSION *in_row_group_ctr,
                             JDIMENSION in_row_groups_avail,
                             JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
                             JDIMENSION out_rows_avail);
};


struct jpeg_marker_reader {
  void (*reset_marker_reader) (j_decompress_ptr cinfo);
  



  int (*read_markers) (j_decompress_ptr cinfo);
  
  jpeg_marker_parser_method read_restart_marker;

  


  boolean saw_SOI;              
  boolean saw_SOF;              
  int next_restart_num;         
  unsigned int discarded_bytes; 
};


struct jpeg_entropy_decoder {
  void (*start_pass) (j_decompress_ptr cinfo);
  boolean (*decode_mcu) (j_decompress_ptr cinfo, JBLOCKROW *MCU_data);

  
  
  boolean insufficient_data;    
};


typedef void (*inverse_DCT_method_ptr) (j_decompress_ptr cinfo,
                                        jpeg_component_info * compptr,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col);

struct jpeg_inverse_dct {
  void (*start_pass) (j_decompress_ptr cinfo);
  
  inverse_DCT_method_ptr inverse_DCT[MAX_COMPONENTS];
};


struct jpeg_upsampler {
  void (*start_pass) (j_decompress_ptr cinfo);
  void (*upsample) (j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                    JDIMENSION *in_row_group_ctr,
                    JDIMENSION in_row_groups_avail, JSAMPARRAY output_buf,
                    JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail);

  boolean need_context_rows;    
};


struct jpeg_color_deconverter {
  void (*start_pass) (j_decompress_ptr cinfo);
  void (*color_convert) (j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                         JDIMENSION input_row, JSAMPARRAY output_buf,
                         int num_rows);
};


struct jpeg_color_quantizer {
  void (*start_pass) (j_decompress_ptr cinfo, boolean is_pre_scan);
  void (*color_quantize) (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
                          JSAMPARRAY output_buf, int num_rows);
  void (*finish_pass) (j_decompress_ptr cinfo);
  void (*new_color_map) (j_decompress_ptr cinfo);
};




#undef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))












#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS     INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
        ((shift_temp = (x)) < 0 ? \
         (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
         (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)     ((x) >> (shft))
#endif



EXTERN(void) jinit_compress_master (j_compress_ptr cinfo);
EXTERN(void) jinit_c_master_control (j_compress_ptr cinfo,
                                     boolean transcode_only);
EXTERN(void) jinit_c_main_controller (j_compress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_c_prep_controller (j_compress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_c_coef_controller (j_compress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_color_converter (j_compress_ptr cinfo);
EXTERN(void) jinit_downsampler (j_compress_ptr cinfo);
EXTERN(void) jinit_forward_dct (j_compress_ptr cinfo);
EXTERN(void) jinit_huff_encoder (j_compress_ptr cinfo);
EXTERN(void) jinit_phuff_encoder (j_compress_ptr cinfo);
EXTERN(void) jinit_arith_encoder (j_compress_ptr cinfo);
EXTERN(void) jinit_marker_writer (j_compress_ptr cinfo);

EXTERN(void) jinit_master_decompress (j_decompress_ptr cinfo);
EXTERN(void) jinit_d_main_controller (j_decompress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_d_coef_controller (j_decompress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_d_post_controller (j_decompress_ptr cinfo,
                                      boolean need_full_buffer);
EXTERN(void) jinit_input_controller (j_decompress_ptr cinfo);
EXTERN(void) jinit_marker_reader (j_decompress_ptr cinfo);
EXTERN(void) jinit_huff_decoder (j_decompress_ptr cinfo);
EXTERN(void) jinit_phuff_decoder (j_decompress_ptr cinfo);
EXTERN(void) jinit_arith_decoder (j_decompress_ptr cinfo);
EXTERN(void) jinit_inverse_dct (j_decompress_ptr cinfo);
EXTERN(void) jinit_upsampler (j_decompress_ptr cinfo);
EXTERN(void) jinit_color_deconverter (j_decompress_ptr cinfo);
EXTERN(void) jinit_1pass_quantizer (j_decompress_ptr cinfo);
EXTERN(void) jinit_2pass_quantizer (j_decompress_ptr cinfo);
EXTERN(void) jinit_merged_upsampler (j_decompress_ptr cinfo);

EXTERN(void) jinit_memory_mgr (j_common_ptr cinfo);


EXTERN(long) jdiv_round_up (long a, long b);
EXTERN(long) jround_up (long a, long b);
EXTERN(void) jcopy_sample_rows (JSAMPARRAY input_array, int source_row,
                                JSAMPARRAY output_array, int dest_row,
                                int num_rows, JDIMENSION num_cols);
EXTERN(void) jcopy_block_row (JBLOCKROW input_row, JBLOCKROW output_row,
                              JDIMENSION num_blocks);
EXTERN(void) jzero_far (void * target, size_t bytestozero);

#if 0                           
extern const int jpeg_zigzag_order[]; 
#endif
extern const int jpeg_natural_order[]; 


extern const INT32 jpeg_aritab[];



#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef AM_MEMORY_MANAGER       
struct jvirt_sarray_control { long dummy; };
struct jvirt_barray_control { long dummy; };
#endif
#endif 
