












































#include <math.h>

#include "gtest/gtest.h"
#include "webrtc/modules/rtp_rtcp/source/forward_error_correction_internal.h"
#include "webrtc/modules/rtp_rtcp/test/testFec/average_residual_loss_xor_codes.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {


enum { kMaxNumberMediaPackets = 48 };


const uint16_t kMaxMediaPackets[] = {kMaxNumberMediaPackets, 12};



const int kMaxMediaPacketsTest = 12;


const int kNumberCodes = kMaxMediaPacketsTest * (kMaxMediaPacketsTest + 1) / 2;


const int kMaxGapSize = 2 * kMaxMediaPacketsTest;


const int kGapSizeOutput = 5;


const int kNumStatesDistribution = 2 * kMaxMediaPacketsTest * kMaxGapSize + 1;


enum CodeType {
  xor_random_code,    
  xor_bursty_code,    
  rs_code             
};


struct CodeSizeParams {
  int num_media_packets;
  int num_fec_packets;
  
  float protection_level;
  
  
  
  int configuration_density[kNumStatesDistribution];
};


enum LossModelType {
  kRandomLossModel,
  kBurstyLossModel
};

struct LossModel {
  LossModelType loss_type;
  float average_loss_rate;
  float average_burst_length;
};


const float kAverageLossRate[] = { 0.025f, 0.05f, 0.1f, 0.25f };





const float kAverageBurstLength[] = { 1.0f, 2.0f, 4.0f };



const int kNumLossModels =  (sizeof(kAverageBurstLength) /
    sizeof(*kAverageBurstLength)) * (sizeof(kAverageLossRate) /
        sizeof(*kAverageLossRate));



float loss_rate_upper_threshold = 0.20f;
float loss_rate_lower_threshold = 0.025f;




const float kRecoveryRateXorRandom[3] = { 0.94f, 0.50f, 0.19f };
const float kRecoveryRateXorBursty[3] = { 0.90f, 0.54f, 0.22f };




struct MetricsFecCode {
  
  
  
  
  double average_residual_loss[kNumLossModels];
  double variance_residual_loss[kNumLossModels];
  
  
  
  double residual_loss_per_loss_gap[kNumStatesDistribution];
  
  double recovery_rate_per_loss[2 * kMaxMediaPacketsTest + 1];
};

MetricsFecCode kMetricsXorRandom[kNumberCodes];
MetricsFecCode kMetricsXorBursty[kNumberCodes];
MetricsFecCode kMetricsReedSolomon[kNumberCodes];

class FecPacketMaskMetricsTest : public ::testing::Test {
 protected:
  FecPacketMaskMetricsTest() { }

  int max_num_codes_;
  LossModel loss_model_[kNumLossModels];
  CodeSizeParams code_params_[kNumberCodes];

  uint8_t fec_packet_masks_[kMaxNumberMediaPackets][kMaxNumberMediaPackets];
  FILE* fp_mask_;

  
  
  
  
  int GapLoss(int tot_num_packets, uint8_t* state) {
    int max_gap_loss = 0;
    
    int first_loss = 0;
    for (int i = 0; i < tot_num_packets; i++) {
      if (state[i] == 1) {
        first_loss = i;
        break;
      }
    }
    int prev_loss = first_loss;
    for (int i = first_loss + 1; i < tot_num_packets; i++) {
      if (state[i] == 1) {  
        int gap_loss = (i - prev_loss) - 1;
        if (gap_loss > max_gap_loss) {
          max_gap_loss = gap_loss;
        }
        prev_loss = i;
      }
    }
    return max_gap_loss;
  }

  
  
  
  int RecoveredMediaPackets(int num_media_packets,
                            int num_fec_packets,
                            uint8_t* state) {
    scoped_array<uint8_t> state_tmp(
        new uint8_t[num_media_packets + num_fec_packets]);
    memcpy(state_tmp.get(), state, num_media_packets + num_fec_packets);
    int num_recovered_packets = 0;
    bool loop_again = true;
    while (loop_again) {
      loop_again = false;
      bool recovered_new_packet = false;
      
      for (int i = 0; i < num_fec_packets; i++) {
        if (state_tmp[i + num_media_packets] == 0) {
          
          int num_packets_in_mask = 0;
          int num_received_packets_in_mask = 0;
          for (int j = 0; j < num_media_packets; j++) {
            if (fec_packet_masks_[i][j] == 1) {
              num_packets_in_mask++;
              if (state_tmp[j] == 0) {
                num_received_packets_in_mask++;
              }
            }
          }
          if ((num_packets_in_mask - 1) == num_received_packets_in_mask) {
            
            num_recovered_packets++;
            recovered_new_packet = true;
            int jsel = -1;
            int check_num_recovered = 0;
            
            for (int j = 0; j < num_media_packets; j++) {
              if (fec_packet_masks_[i][j] == 1 && state_tmp[j] == 1) {
                
                jsel = j;
                check_num_recovered++;
              }
            }
            
            assert(check_num_recovered == 1);
            
            state_tmp[jsel] = 0;
          }
        }
      }  
      
      
      if (recovered_new_packet) {
        loop_again = true;
      }
    }
    return num_recovered_packets;
  }

  
  
  void ComputeProbabilityWeight(double* prob_weight,
                                uint8_t* state,
                                int tot_num_packets) {
    
    for (int k = 0; k < kNumLossModels; k++) {
      double loss_rate = static_cast<double>(
          loss_model_[k].average_loss_rate);
      double burst_length = static_cast<double>(
          loss_model_[k].average_burst_length);
      double result = 1.0;
      if (loss_model_[k].loss_type == kRandomLossModel) {
        for (int i = 0; i < tot_num_packets; i++) {
          if (state[i] == 0) {
            result *= (1.0 - loss_rate);
          } else {
            result *= loss_rate;
          }
        }
      } else {  
        assert(loss_model_[k].loss_type == kBurstyLossModel);
        
        
        double prob10 = 1.0 / burst_length;
        
        double prob11 = 1.0 - prob10;
        
        double prob01 = prob10 * (loss_rate / (1.0 - loss_rate));
        
        double prob00 = 1.0 - prob01;

        
        if (state[0] == 0) {  
          result = (1.0 - loss_rate);
        } else {   
          result = loss_rate;
        }

        
        for (int i = 1; i < tot_num_packets; i++) {
          
          if (state[i] == 0) {
            if (state[i-1] == 0) {
              result *= prob00;   
              } else {
                result *= prob10;  
              }
          } else {  
            if (state[i-1] == 0) {
              result *= prob01;  
            } else {
              result *= prob11;  
            }
          }
        }
      }
      prob_weight[k] = result;
    }
  }

  void CopyMetrics(MetricsFecCode* metrics_output,
                   MetricsFecCode metrics_input) {
    memcpy(metrics_output->average_residual_loss,
           metrics_input.average_residual_loss,
           sizeof(double) * kNumLossModels);
    memcpy(metrics_output->variance_residual_loss,
           metrics_input.variance_residual_loss,
           sizeof(double) * kNumLossModels);
    memcpy(metrics_output->residual_loss_per_loss_gap,
           metrics_input.residual_loss_per_loss_gap,
           sizeof(double) * kNumStatesDistribution);
    memcpy(metrics_output->recovery_rate_per_loss,
           metrics_input.recovery_rate_per_loss,
           sizeof(double) * 2 * kMaxMediaPacketsTest);
  }

  
  
  
  double ComputeResidualLossPerGap(MetricsFecCode metrics,
                                   int gap_number,
                                   int num_fec_packets,
                                   int code_index) {
    double residual_loss_gap = 0.0;
    int tot_num_configs = 0;
    for (int loss = 1; loss <= num_fec_packets; loss++) {
      int index = gap_number * (2 * kMaxMediaPacketsTest) + loss;
      residual_loss_gap += metrics.residual_loss_per_loss_gap[index];
      tot_num_configs +=
          code_params_[code_index].configuration_density[index];
    }
    
    if (tot_num_configs > 0) {
      residual_loss_gap = residual_loss_gap /
          static_cast<double>(tot_num_configs);
    }
    return residual_loss_gap;
  }

  
  
  void ComputeRecoveryRatePerLoss(MetricsFecCode* metrics,
                                  int num_media_packets,
                                  int num_fec_packets,
                                  int code_index) {
    for (int loss = 1; loss <= num_media_packets + num_fec_packets; loss++) {
      metrics->recovery_rate_per_loss[loss] = 0.0;
      int tot_num_configs = 0;
      double arl = 0.0;
      for (int gap = 0; gap < kMaxGapSize; gap ++) {
        int index = gap * (2 * kMaxMediaPacketsTest) + loss;
        arl += metrics->residual_loss_per_loss_gap[index];
        tot_num_configs +=
            code_params_[code_index].configuration_density[index];
      }
      
      if (tot_num_configs > 0) {
        arl = arl / static_cast<double>(tot_num_configs);
      }
      
      
      double scaled_loss = static_cast<double>(loss * num_media_packets) /
          static_cast<double>(num_media_packets + num_fec_packets);
      metrics->recovery_rate_per_loss[loss] = 1.0 - arl / scaled_loss;
    }
  }

  void SetMetricsZero(MetricsFecCode* metrics) {
    memset(metrics->average_residual_loss, 0, sizeof(double) * kNumLossModels);
    memset(metrics->variance_residual_loss, 0, sizeof(double) * kNumLossModels);
    memset(metrics->residual_loss_per_loss_gap, 0,
           sizeof(double) * kNumStatesDistribution);
    memset(metrics->recovery_rate_per_loss, 0,
           sizeof(double) * 2 * kMaxMediaPacketsTest + 1);
  }

  
  
  
  void ComputeMetricsForCode(CodeType code_type,
                             int code_index) {
    scoped_array<double> prob_weight(new double[kNumLossModels]);
    memset(prob_weight.get() , 0, sizeof(double) * kNumLossModels);
    MetricsFecCode metrics_code;
    SetMetricsZero(&metrics_code);

    int num_media_packets = code_params_[code_index].num_media_packets;
    int num_fec_packets = code_params_[code_index].num_fec_packets;
    int tot_num_packets = num_media_packets + num_fec_packets;
    scoped_array<uint8_t> state(new uint8_t[tot_num_packets]);
    memset(state.get() , 0, tot_num_packets);

    int num_loss_configurations = static_cast<int>(pow(2.0f, tot_num_packets));
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    for (int i = 1; i < num_loss_configurations; i++) {
      
      int num_packets_lost = 0;
      
      int num_media_packets_lost = 0;

      
      for (int j = 0; j < tot_num_packets; j++) {
        state[j]=0;  
        int bit_value = i >> (tot_num_packets - j - 1) & 1;
        if (bit_value == 1) {
          state[j] = 1;  
          num_packets_lost++;
           if (j < num_media_packets) {
             num_media_packets_lost++;
           }
        }
      }  
      assert(num_media_packets_lost <= num_media_packets);
      assert(num_packets_lost <= tot_num_packets && num_packets_lost > 0);
      double residual_loss = 0.0;
      
      
      if (num_media_packets_lost >= 1) {
        
        int num_recovered_packets = 0;
        if (code_type == xor_random_code || code_type == xor_bursty_code) {
          num_recovered_packets = RecoveredMediaPackets(num_media_packets,
                                                        num_fec_packets,
                                                        state.get());
        } else {
          
          
          
          
          if (num_packets_lost <= num_fec_packets) {
            num_recovered_packets = num_media_packets_lost;
          }
        }
        assert(num_recovered_packets <= num_media_packets);
        
        
        residual_loss = static_cast<double>(num_media_packets_lost -
                                            num_recovered_packets);
        
        ComputeProbabilityWeight(prob_weight.get(),
                                 state.get(),
                                 tot_num_packets);
        
        for (int k = 0; k < kNumLossModels; k++) {
          metrics_code.average_residual_loss[k] += residual_loss *
              prob_weight[k];
          metrics_code.variance_residual_loss[k] += residual_loss *
              residual_loss * prob_weight[k];
        }
      }  
      
      
      int gap_loss = GapLoss(tot_num_packets, state.get());
      assert(gap_loss < kMaxGapSize);
      int index = gap_loss * (2 * kMaxMediaPacketsTest) + num_packets_lost;
      assert(index < kNumStatesDistribution);
      metrics_code.residual_loss_per_loss_gap[index] += residual_loss;
      if (code_type == xor_random_code) {
        
        
        code_params_[code_index].configuration_density[index]++;
      }
    }  
    
    for (int k = 0; k < kNumLossModels; k++) {
      
      
      
      
      
      
      metrics_code.average_residual_loss[k] =
          metrics_code.average_residual_loss[k] /
          static_cast<double>(tot_num_packets);
      metrics_code.variance_residual_loss[k] =
               metrics_code.variance_residual_loss[k] /
               static_cast<double>(num_media_packets * num_media_packets);
      metrics_code.variance_residual_loss[k] =
          metrics_code.variance_residual_loss[k] -
          (metrics_code.average_residual_loss[k] *
              metrics_code.average_residual_loss[k]);
      assert(metrics_code.variance_residual_loss[k] >= 0.0);
      assert(metrics_code.average_residual_loss[k] > 0.0);
      metrics_code.variance_residual_loss[k] =
          sqrt(metrics_code.variance_residual_loss[k]) /
          metrics_code.average_residual_loss[k];
    }

    
    ComputeRecoveryRatePerLoss(&metrics_code,
                               num_media_packets,
                               num_fec_packets,
                               code_index);
    if (code_type == rs_code) {
      CopyMetrics(&kMetricsReedSolomon[code_index], metrics_code);
    } else if (code_type == xor_random_code) {
      CopyMetrics(&kMetricsXorRandom[code_index], metrics_code);
    } else if (code_type == xor_bursty_code) {
      CopyMetrics(&kMetricsXorBursty[code_index], metrics_code);
    } else {
      assert(false);
    }
  }

  void WriteOutMetricsAllFecCodes()  {
    std::string filename = test::OutputPath() + "data_metrics_all_codes";
    FILE* fp = fopen(filename.c_str(), "wb");
    
    int code_index = 0;
    for (int num_media_packets = 1; num_media_packets <= kMaxMediaPacketsTest;
        num_media_packets++) {
      for (int num_fec_packets = 1; num_fec_packets <= num_media_packets;
          num_fec_packets++) {
        fprintf(fp, "FOR CODE: (%d, %d) \n", num_media_packets,
                num_fec_packets);
        for (int k = 0; k < kNumLossModels; k++) {
          float loss_rate = loss_model_[k].average_loss_rate;
          float burst_length = loss_model_[k].average_burst_length;
          fprintf(fp, "Loss rate = %.2f, Burst length = %.2f:  %.4f  %.4f  %.4f"
              " **** %.4f %.4f %.4f \n",
              loss_rate,
              burst_length,
              100 * kMetricsReedSolomon[code_index].average_residual_loss[k],
              100 * kMetricsXorRandom[code_index].average_residual_loss[k],
              100 * kMetricsXorBursty[code_index].average_residual_loss[k],
              kMetricsReedSolomon[code_index].variance_residual_loss[k],
              kMetricsXorRandom[code_index].variance_residual_loss[k],
              kMetricsXorBursty[code_index].variance_residual_loss[k]);
        }
        for (int gap = 0; gap < kGapSizeOutput; gap ++) {
          double rs_residual_loss = ComputeResidualLossPerGap(
              kMetricsReedSolomon[code_index],
              gap,
              num_fec_packets,
              code_index);
          double xor_random_residual_loss = ComputeResidualLossPerGap(
              kMetricsXorRandom[code_index],
              gap,
              num_fec_packets,
              code_index);
          double xor_bursty_residual_loss = ComputeResidualLossPerGap(
              kMetricsXorBursty[code_index],
              gap,
              num_fec_packets,
              code_index);
          fprintf(fp, "Residual loss as a function of gap "
              "%d: %.4f %.4f %.4f \n",
              gap,
              rs_residual_loss,
              xor_random_residual_loss,
              xor_bursty_residual_loss);
        }
        fprintf(fp, "Recovery rate as a function of loss number \n");
        for (int loss = 1; loss <= num_media_packets + num_fec_packets;
                     loss ++) {
          fprintf(fp, "For loss number %d: %.4f %.4f %.4f \n",
                  loss,
                  kMetricsReedSolomon[code_index].
                  recovery_rate_per_loss[loss],
                  kMetricsXorRandom[code_index].
                  recovery_rate_per_loss[loss],
                  kMetricsXorBursty[code_index].
                  recovery_rate_per_loss[loss]);
        }
        fprintf(fp, "******************\n");
        fprintf(fp, "\n");
        code_index++;
      }
    }
    fclose(fp);
  }

  void SetLossModels() {
    int num_loss_rates = sizeof(kAverageLossRate) /
        sizeof(*kAverageLossRate);
    int num_burst_lengths = sizeof(kAverageBurstLength) /
        sizeof(*kAverageBurstLength);
    int num_loss_models = 0;
    for (int k = 0; k < num_burst_lengths; k++) {
      for (int k2 = 0; k2 < num_loss_rates; k2++) {
        loss_model_[num_loss_models].average_loss_rate = kAverageLossRate[k2];
        loss_model_[num_loss_models].average_burst_length =
            kAverageBurstLength[k];
        
        if (k == 0) {
          loss_model_[num_loss_models].loss_type = kRandomLossModel;
        } else {
          loss_model_[num_loss_models].loss_type = kBurstyLossModel;
        }
        num_loss_models++;
      }
    }
    assert(num_loss_models == kNumLossModels);
  }

  void SetCodeParams() {
    int code_index = 0;
    for (int num_media_packets = 1; num_media_packets <= kMaxMediaPacketsTest;
        num_media_packets++) {
      for (int num_fec_packets = 1; num_fec_packets <= num_media_packets;
          num_fec_packets++) {
        code_params_[code_index].num_media_packets = num_media_packets;
        code_params_[code_index].num_fec_packets = num_fec_packets;
        code_params_[code_index].protection_level =
            static_cast<float>(num_fec_packets) /
            static_cast<float>(num_media_packets + num_fec_packets);
        for (int k = 0; k < kNumStatesDistribution; k++) {
          code_params_[code_index].configuration_density[k] = 0;
        }
        code_index++;
      }
    }
    max_num_codes_ = code_index;
  }

  
  
  int RejectInvalidMasks(int num_media_packets, int num_fec_packets) {
    
    for (int i = 0; i < num_fec_packets; i++) {
      int row_degree = 0;
      for (int j = 0; j < num_media_packets; j++) {
        if (fec_packet_masks_[i][j] == 1) {
          row_degree++;
        }
      }
      if (row_degree == 0) {
        printf("Invalid mask: FEC packet has empty mask (does not protect "
            "anything) %d %d %d \n", i, num_media_packets, num_fec_packets);
        return -1;
      }
    }
    
    for (int j = 0; j < num_media_packets; j++) {
      int column_degree = 0;
      for (int i = 0; i < num_fec_packets; i++) {
        if (fec_packet_masks_[i][j] == 1) {
          column_degree++;
        }
      }
      if (column_degree == 0) {
        printf("Invalid mask: Media packet has no protection at all %d %d %d "
            "\n", j, num_media_packets, num_fec_packets);
        return -1;
      }
    }
    
    for (int i = 0; i < num_fec_packets; i++) {
      for (int i2 = i + 1; i2 < num_fec_packets; i2++) {
        int overlap = 0;
        for (int j = 0; j < num_media_packets; j++) {
          if (fec_packet_masks_[i][j] == fec_packet_masks_[i2][j]) {
            overlap++;
          }
        }
        if (overlap == num_media_packets) {
          printf("Invalid mask: Two FEC packets are identical %d %d %d %d \n",
                 i, i2, num_media_packets, num_fec_packets);
          return -1;
        }
      }
    }
    
    
    
    
    if (num_fec_packets > 2) {
      for (int j = 0; j < num_media_packets; j++) {
        for (int j2 = j + 1; j2 < num_media_packets; j2++) {
          int degree = 0;
          for (int i = 0; i < num_fec_packets; i++) {
            if (fec_packet_masks_[i][j] == fec_packet_masks_[i][j2] &&
                fec_packet_masks_[i][j] == 1) {
              degree++;
            }
          }
          if (degree == num_fec_packets) {
            printf("Invalid mask: Two media packets are have full degree "
                "%d %d %d %d \n", j, j2, num_media_packets, num_fec_packets);
            return -1;
          }
        }
      }
    }
    return 0;
  }

  void GetPacketMaskConvertToBitMask(uint8_t* packet_mask,
                                     int num_media_packets,
                                     int num_fec_packets,
                                     int mask_bytes_fec_packet,
                                     CodeType code_type) {
    for (int i = 0; i < num_fec_packets; i++) {
      for (int j = 0; j < num_media_packets; j++) {
        const uint8_t byte_mask =
            packet_mask[i * mask_bytes_fec_packet + j / 8];
        const int bit_position = (7 - j % 8);
        fec_packet_masks_[i][j] =
            (byte_mask & (1 << bit_position)) >> bit_position;
        fprintf(fp_mask_, "%d ", fec_packet_masks_[i][j]);
      }
      fprintf(fp_mask_, "\n");
    }
    fprintf(fp_mask_, "\n");
  }

  int ProcessXORPacketMasks(CodeType code_type,
                          FecMaskType fec_mask_type) {
    int code_index = 0;
    
    const int packet_mask_max = kMaxMediaPackets[fec_mask_type];
    uint8_t* packet_mask = new uint8_t[packet_mask_max * kMaskSizeLBitSet];
    
    for (int num_media_packets = 1; num_media_packets <= kMaxMediaPacketsTest;
        num_media_packets++) {
      const int mask_bytes_fec_packet =
          (num_media_packets > 16) ? kMaskSizeLBitSet : kMaskSizeLBitClear;
      internal::PacketMaskTable mask_table(fec_mask_type, num_media_packets);
      for (int num_fec_packets = 1; num_fec_packets <= num_media_packets;
          num_fec_packets++) {
        memset(packet_mask, 0, num_media_packets * mask_bytes_fec_packet);
        memcpy(packet_mask, mask_table.fec_packet_mask_table()
               [num_media_packets - 1][num_fec_packets - 1],
               num_fec_packets * mask_bytes_fec_packet);
        
        GetPacketMaskConvertToBitMask(packet_mask,
                                      num_media_packets,
                                      num_fec_packets,
                                      mask_bytes_fec_packet,
                                      code_type);
        if (RejectInvalidMasks(num_media_packets, num_fec_packets) < 0) {
          return -1;
        }
        
        ComputeMetricsForCode(code_type,
                              code_index);
        code_index++;
      }
    }
    assert(code_index == kNumberCodes);
    delete [] packet_mask;
    return 0;
  }

  void ProcessRS(CodeType code_type) {
    int code_index = 0;
    for (int num_media_packets = 1; num_media_packets <= kMaxMediaPacketsTest;
        num_media_packets++) {
      for (int num_fec_packets = 1; num_fec_packets <= num_media_packets;
          num_fec_packets++) {
        
        ComputeMetricsForCode(code_type,
                              code_index);
        code_index++;
      }
    }
  }

  
  void ComputeMetricsAllCodes() {
    SetLossModels();
    SetCodeParams();
    
    std::string filename = test::OutputPath() + "data_packet_masks";
    fp_mask_ = fopen(filename.c_str(), "wb");
    fprintf(fp_mask_, "MASK OF TYPE RANDOM: \n");
    EXPECT_EQ(ProcessXORPacketMasks(xor_random_code, kFecMaskRandom), 0);
    
    fprintf(fp_mask_, "MASK OF TYPE BURSTY: \n");
    EXPECT_EQ(ProcessXORPacketMasks(xor_bursty_code, kFecMaskBursty), 0);
    fclose(fp_mask_);
    
    ProcessRS(rs_code);
  }
};





TEST_F(FecPacketMaskMetricsTest, FecXorMaxResidualLoss) {
  SetLossModels();
  SetCodeParams();
  ComputeMetricsAllCodes();
  WriteOutMetricsAllFecCodes();
  int num_loss_rates = sizeof(kAverageLossRate) /
      sizeof(*kAverageLossRate);
  int num_burst_lengths = sizeof(kAverageBurstLength) /
      sizeof(*kAverageBurstLength);
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    double sum_residual_loss_random_mask_random_loss = 0.0;
    double sum_residual_loss_bursty_mask_bursty_loss = 0.0;
    
    for (int k = 0; k < kNumLossModels; k++) {
      if (loss_model_[k].loss_type == kRandomLossModel) {
        sum_residual_loss_random_mask_random_loss +=
            kMetricsXorRandom[code_index].average_residual_loss[k];
      } else if (loss_model_[k].loss_type == kBurstyLossModel) {
        sum_residual_loss_bursty_mask_bursty_loss +=
            kMetricsXorBursty[code_index].average_residual_loss[k];
      }
    }
    float average_residual_loss_random_mask_random_loss =
        sum_residual_loss_random_mask_random_loss / num_loss_rates;
    float average_residual_loss_bursty_mask_bursty_loss =
        sum_residual_loss_bursty_mask_bursty_loss /
        (num_loss_rates * (num_burst_lengths  - 1));
    const float ref_random_mask = kMaxResidualLossRandomMask[code_index];
    const float ref_bursty_mask = kMaxResidualLossBurstyMask[code_index];
    EXPECT_LE(average_residual_loss_random_mask_random_loss, ref_random_mask);
    EXPECT_LE(average_residual_loss_bursty_mask_bursty_loss, ref_bursty_mask);
  }
}







TEST_F(FecPacketMaskMetricsTest, FecXorVsRS) {
  SetLossModels();
  SetCodeParams();
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    for (int k = 0; k < kNumLossModels; k++) {
      float loss_rate = loss_model_[k].average_loss_rate;
      float protection_level = code_params_[code_index].protection_level;
      
       if (loss_model_[k].loss_type == kRandomLossModel &&
           loss_rate <= protection_level) {
        EXPECT_GE(kMetricsXorRandom[code_index].average_residual_loss[k],
                  kMetricsReedSolomon[code_index].average_residual_loss[k]);
        EXPECT_GE(kMetricsXorBursty[code_index].average_residual_loss[k],
                  kMetricsReedSolomon[code_index].average_residual_loss[k]);
       }
      
      
      
    }
  }
}





TEST_F(FecPacketMaskMetricsTest, FecTrendXorVsRsLossRate) {
  SetLossModels();
  SetCodeParams();
  
  
  double scale = 0.90;
  int num_loss_rates = sizeof(kAverageLossRate) /
      sizeof(*kAverageLossRate);
  int num_burst_lengths = sizeof(kAverageBurstLength) /
      sizeof(*kAverageBurstLength);
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    for (int i = 0; i < num_burst_lengths; i++) {
      for (int j = 0; j < num_loss_rates - 1; j++) {
        int k = num_loss_rates * i + j;
        
        if (kMetricsXorRandom[code_index].average_residual_loss[k] >
        kMetricsReedSolomon[code_index].average_residual_loss[k]) {
          double diff_rs_xor_random_loss1 =
              (kMetricsXorRandom[code_index].average_residual_loss[k] -
               kMetricsReedSolomon[code_index].average_residual_loss[k]) /
               kMetricsXorRandom[code_index].average_residual_loss[k];
          double diff_rs_xor_random_loss2 =
              (kMetricsXorRandom[code_index].average_residual_loss[k+1] -
               kMetricsReedSolomon[code_index].average_residual_loss[k+1]) /
               kMetricsXorRandom[code_index].average_residual_loss[k+1];
          EXPECT_GE(diff_rs_xor_random_loss1, scale * diff_rs_xor_random_loss2);
        }
        
        
      }
    }
  }
}








TEST_F(FecPacketMaskMetricsTest, FecBehaviorViaProtectionLevelAndLength) {
  SetLossModels();
  SetCodeParams();
  for (int code_index1 = 0; code_index1 < max_num_codes_; code_index1++) {
    float protection_level1 = code_params_[code_index1].protection_level;
    int length1 = code_params_[code_index1].num_media_packets +
        code_params_[code_index1].num_fec_packets;
    for (int code_index2 = 0; code_index2 < max_num_codes_; code_index2++) {
      float protection_level2 = code_params_[code_index2].protection_level;
      int length2 = code_params_[code_index2].num_media_packets +
          code_params_[code_index2].num_fec_packets;
      
      
      
      
      
      
      if ((protection_level2 > protection_level1 && length2 >= length1) ||
          (protection_level2 == protection_level1 && length2 > length1)) {
        for (int k = 0; k < kNumLossModels; k++) {
          float loss_rate = loss_model_[k].average_loss_rate;
          if (loss_rate < loss_rate_upper_threshold) {
            EXPECT_LT(
                kMetricsReedSolomon[code_index2].average_residual_loss[k],
                kMetricsReedSolomon[code_index1].average_residual_loss[k]);
            
            
            
            
            
            
            
            
            
            
          }
        }
      }
    }
  }
}





TEST_F(FecPacketMaskMetricsTest, FecVarianceBehaviorXorVsRs) {
  SetLossModels();
  SetCodeParams();
  
  
  
  
  double scale = 0.95;
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    for (int k = 0; k < kNumLossModels; k++) {
      EXPECT_LE(scale *
                kMetricsXorRandom[code_index].variance_residual_loss[k],
                kMetricsReedSolomon[code_index].variance_residual_loss[k]);
      EXPECT_LE(scale *
                kMetricsXorBursty[code_index].variance_residual_loss[k],
                kMetricsReedSolomon[code_index].variance_residual_loss[k]);
    }
  }
}




TEST_F(FecPacketMaskMetricsTest, FecXorBurstyPerfectRecoveryConsecutiveLoss) {
  SetLossModels();
  SetCodeParams();
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    int num_fec_packets = code_params_[code_index].num_fec_packets;
    for (int loss = 1; loss <= num_fec_packets; loss++) {
      int index = loss;  
      EXPECT_EQ(kMetricsXorBursty[code_index].
                residual_loss_per_loss_gap[index], 0.0);
    }
  }
}











































TEST_F(FecPacketMaskMetricsTest, FecRecoveryRateUnderLossConditions) {
  SetLossModels();
  SetCodeParams();
  for (int code_index = 0; code_index < max_num_codes_; code_index++) {
    int num_media_packets = code_params_[code_index].num_media_packets;
    int num_fec_packets = code_params_[code_index].num_fec_packets;
    
    
    int loss_number = 1;
    EXPECT_EQ(kMetricsReedSolomon[code_index].
              recovery_rate_per_loss[loss_number], 1.0);
    EXPECT_EQ(kMetricsXorRandom[code_index].
              recovery_rate_per_loss[loss_number], 1.0);
    EXPECT_EQ(kMetricsXorBursty[code_index].
              recovery_rate_per_loss[loss_number], 1.0);
    
    
    loss_number = num_fec_packets / 2 > 0 ? num_fec_packets / 2 : 1;
    EXPECT_EQ(kMetricsReedSolomon[code_index].
              recovery_rate_per_loss[loss_number], 1.0);
    EXPECT_GE(kMetricsXorRandom[code_index].
              recovery_rate_per_loss[loss_number], kRecoveryRateXorRandom[0]);
    EXPECT_GE(kMetricsXorBursty[code_index].
              recovery_rate_per_loss[loss_number], kRecoveryRateXorBursty[0]);
    
    
    loss_number = num_fec_packets;
    EXPECT_EQ(kMetricsReedSolomon[code_index].
              recovery_rate_per_loss[loss_number], 1.0);
    EXPECT_GE(kMetricsXorRandom[code_index].
              recovery_rate_per_loss[loss_number], kRecoveryRateXorRandom[1]);
    EXPECT_GE(kMetricsXorBursty[code_index].
              recovery_rate_per_loss[loss_number], kRecoveryRateXorBursty[1]);
    
    
    if (num_fec_packets > 1 && num_media_packets > 2) {
      loss_number =  num_fec_packets + 1;
      EXPECT_EQ(kMetricsReedSolomon[code_index].
                recovery_rate_per_loss[loss_number], 0.0);
      EXPECT_GE(kMetricsXorRandom[code_index].
                recovery_rate_per_loss[loss_number],
                kRecoveryRateXorRandom[2]);
      EXPECT_GE(kMetricsXorBursty[code_index].
                recovery_rate_per_loss[loss_number],
                kRecoveryRateXorBursty[2]);
    }
  }
}

}  
