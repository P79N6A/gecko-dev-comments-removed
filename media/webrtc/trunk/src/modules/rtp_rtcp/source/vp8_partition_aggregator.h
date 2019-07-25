









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_VP8_PARTITION_AGGREGATOR_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_VP8_PARTITION_AGGREGATOR_H_

#include <vector>

#include "modules/interface/module_common_types.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "typedefs.h"  

namespace webrtc {


class PartitionTreeNode {
 public:
  
  PartitionTreeNode(PartitionTreeNode* parent,
                    const int* size_vector,
                    int num_partitions,
                    int this_size);

  
  static PartitionTreeNode* CreateRootNode(const int* size_vector,
                                           int num_partitions);

  ~PartitionTreeNode();

  
  
  
  
  int Cost(int penalty);

  
  bool CreateChildren(int max_size);

  
  int NumPackets();

  
  
  
  PartitionTreeNode* GetOptimalNode(int max_size, int penalty);

  
  void set_max_parent_size(int size) { max_parent_size_ = size; }
  void set_min_parent_size(int size) { min_parent_size_ = size; }
  PartitionTreeNode* parent() const { return parent_; }
  PartitionTreeNode* left_child() const { return children_[kLeftChild]; }
  PartitionTreeNode* right_child() const { return children_[kRightChild]; }
  int this_size() const { return this_size_; }
  bool packet_start() const { return packet_start_; }

 private:
  enum Children {
    kLeftChild = 0,
    kRightChild = 1
  };

  void set_packet_start(bool value) { packet_start_ = value; }

  PartitionTreeNode* parent_;
  PartitionTreeNode* children_[2];
  int this_size_;
  const int* size_vector_;
  int num_partitions_;
  int max_parent_size_;
  int min_parent_size_;
  bool packet_start_;

  DISALLOW_COPY_AND_ASSIGN(PartitionTreeNode);
};



class Vp8PartitionAggregator {
 public:
  typedef std::vector<int> ConfigVec;

  
  
  
  Vp8PartitionAggregator(const RTPFragmentationHeader& fragmentation,
                         int first_partition_idx, int last_partition_idx);

  ~Vp8PartitionAggregator();

  
  void SetPriorMinMax(int min_size, int max_size);

  
  
  
  
  
  
  ConfigVec FindOptimalConfiguration(int max_size, int penalty);

  
  
  
  
  void CalcMinMax(const ConfigVec& config, int* min_size, int* max_size) const;

  
  
  
  
  
  static int CalcNumberOfFragments(int large_partition_size,
                                   int max_payload_size,
                                   int penalty,
                                   int min_size,
                                   int max_size);

 private:
  PartitionTreeNode* root_;
  size_t num_partitions_;
  int* size_vector_;
  int largest_partition_size_;

  DISALLOW_COPY_AND_ASSIGN(Vp8PartitionAggregator);
};
}  

#endif
