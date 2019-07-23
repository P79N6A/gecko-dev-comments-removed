




























#ifndef COMMON_MAC_DWARF_LINE_STATE_MACHINE_H__
#define COMMON_MAC_DWARF_LINE_STATE_MACHINE_H__

namespace dwarf2reader {




struct LineStateMachine {
  void Reset(bool default_is_stmt) {
    file_num = 1;
    address = 0;
    line_num = 1;
    column_num = 0;
    is_stmt = default_is_stmt;
    basic_block = false;
    end_sequence = false;
  }

  uint32 file_num;
  uint64 address;
  uint64 line_num;
  uint32 column_num;
  bool is_stmt;  
  bool basic_block;
  bool end_sequence;
};

}  


#endif  
