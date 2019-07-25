




#ifndef prerr_h___
#define prerr_h___








#define PR_OUT_OF_MEMORY_ERROR                   (-6000L)


#define PR_BAD_DESCRIPTOR_ERROR                  (-5999L)


#define PR_WOULD_BLOCK_ERROR                     (-5998L)


#define PR_ACCESS_FAULT_ERROR                    (-5997L)


#define PR_INVALID_METHOD_ERROR                  (-5996L)


#define PR_ILLEGAL_ACCESS_ERROR                  (-5995L)


#define PR_UNKNOWN_ERROR                         (-5994L)


#define PR_PENDING_INTERRUPT_ERROR               (-5993L)


#define PR_NOT_IMPLEMENTED_ERROR                 (-5992L)


#define PR_IO_ERROR                              (-5991L)


#define PR_IO_TIMEOUT_ERROR                      (-5990L)


#define PR_IO_PENDING_ERROR                      (-5989L)


#define PR_DIRECTORY_OPEN_ERROR                  (-5988L)


#define PR_INVALID_ARGUMENT_ERROR                (-5987L)


#define PR_ADDRESS_NOT_AVAILABLE_ERROR           (-5986L)


#define PR_ADDRESS_NOT_SUPPORTED_ERROR           (-5985L)


#define PR_IS_CONNECTED_ERROR                    (-5984L)


#define PR_BAD_ADDRESS_ERROR                     (-5983L)


#define PR_ADDRESS_IN_USE_ERROR                  (-5982L)


#define PR_CONNECT_REFUSED_ERROR                 (-5981L)


#define PR_NETWORK_UNREACHABLE_ERROR             (-5980L)


#define PR_CONNECT_TIMEOUT_ERROR                 (-5979L)


#define PR_NOT_CONNECTED_ERROR                   (-5978L)


#define PR_LOAD_LIBRARY_ERROR                    (-5977L)


#define PR_UNLOAD_LIBRARY_ERROR                  (-5976L)


#define PR_FIND_SYMBOL_ERROR                     (-5975L)


#define PR_INSUFFICIENT_RESOURCES_ERROR          (-5974L)


#define PR_DIRECTORY_LOOKUP_ERROR                (-5973L)


#define PR_TPD_RANGE_ERROR                       (-5972L)


#define PR_PROC_DESC_TABLE_FULL_ERROR            (-5971L)


#define PR_SYS_DESC_TABLE_FULL_ERROR             (-5970L)


#define PR_NOT_SOCKET_ERROR                      (-5969L)


#define PR_NOT_TCP_SOCKET_ERROR                  (-5968L)


#define PR_SOCKET_ADDRESS_IS_BOUND_ERROR         (-5967L)


#define PR_NO_ACCESS_RIGHTS_ERROR                (-5966L)


#define PR_OPERATION_NOT_SUPPORTED_ERROR         (-5965L)


#define PR_PROTOCOL_NOT_SUPPORTED_ERROR          (-5964L)


#define PR_REMOTE_FILE_ERROR                     (-5963L)


#define PR_BUFFER_OVERFLOW_ERROR                 (-5962L)


#define PR_CONNECT_RESET_ERROR                   (-5961L)


#define PR_RANGE_ERROR                           (-5960L)


#define PR_DEADLOCK_ERROR                        (-5959L)


#define PR_FILE_IS_LOCKED_ERROR                  (-5958L)


#define PR_FILE_TOO_BIG_ERROR                    (-5957L)


#define PR_NO_DEVICE_SPACE_ERROR                 (-5956L)


#define PR_PIPE_ERROR                            (-5955L)


#define PR_NO_SEEK_DEVICE_ERROR                  (-5954L)


#define PR_IS_DIRECTORY_ERROR                    (-5953L)


#define PR_LOOP_ERROR                            (-5952L)


#define PR_NAME_TOO_LONG_ERROR                   (-5951L)


#define PR_FILE_NOT_FOUND_ERROR                  (-5950L)


#define PR_NOT_DIRECTORY_ERROR                   (-5949L)


#define PR_READ_ONLY_FILESYSTEM_ERROR            (-5948L)


#define PR_DIRECTORY_NOT_EMPTY_ERROR             (-5947L)


#define PR_FILESYSTEM_MOUNTED_ERROR              (-5946L)


#define PR_NOT_SAME_DEVICE_ERROR                 (-5945L)


#define PR_DIRECTORY_CORRUPTED_ERROR             (-5944L)


#define PR_FILE_EXISTS_ERROR                     (-5943L)


#define PR_MAX_DIRECTORY_ENTRIES_ERROR           (-5942L)


#define PR_INVALID_DEVICE_STATE_ERROR            (-5941L)


#define PR_DEVICE_IS_LOCKED_ERROR                (-5940L)


#define PR_NO_MORE_FILES_ERROR                   (-5939L)


#define PR_END_OF_FILE_ERROR                     (-5938L)


#define PR_FILE_SEEK_ERROR                       (-5937L)


#define PR_FILE_IS_BUSY_ERROR                    (-5936L)


#define PR_OPERATION_ABORTED_ERROR               (-5935L)


#define PR_IN_PROGRESS_ERROR                     (-5934L)


#define PR_ALREADY_INITIATED_ERROR               (-5933L)


#define PR_GROUP_EMPTY_ERROR                     (-5932L)


#define PR_INVALID_STATE_ERROR                   (-5931L)


#define PR_NETWORK_DOWN_ERROR                    (-5930L)


#define PR_SOCKET_SHUTDOWN_ERROR                 (-5929L)


#define PR_CONNECT_ABORTED_ERROR                 (-5928L)


#define PR_HOST_UNREACHABLE_ERROR                (-5927L)


#define PR_LIBRARY_NOT_LOADED_ERROR              (-5926L)


#define PR_CALL_ONCE_ERROR                       (-5925L)


#define PR_MAX_ERROR                             (-5924L)

extern void nspr_InitializePRErrorTable(void);
#define ERROR_TABLE_BASE_nspr (-6000L)

#endif 
