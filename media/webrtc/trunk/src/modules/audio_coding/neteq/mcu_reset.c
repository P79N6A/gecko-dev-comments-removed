













#include "mcu.h"

#include <assert.h>
#include <string.h>

#include "automode.h"

int WebRtcNetEQ_McuReset(MCUInst_t *inst)
{

#ifdef NETEQ_ATEVENT_DECODE
    int ok;
#endif

    
    inst->pw16_readAddress = NULL;
    inst->pw16_writeAddress = NULL;
    inst->main_inst = NULL;
    inst->one_desc = 0;
    inst->BufferStat_inst.Automode_inst.extraDelayMs = 0;
    inst->NetEqPlayoutMode = kPlayoutOn;

    WebRtcNetEQ_DbReset(&inst->codec_DB_inst);
    memset(&inst->PayloadSplit_inst, 0, sizeof(SplitInfo_t));

    
    WebRtcNetEQ_PacketBufferFlush(&inst->PacketBuffer_inst);
    inst->PacketBuffer_inst.memorySizeW16 = 0;
    inst->PacketBuffer_inst.maxInsertPositions = 0;

    
    memset(&inst->BufferStat_inst, 0, sizeof(BufstatsInst_t));
#ifdef NETEQ_ATEVENT_DECODE
    ok = WebRtcNetEQ_DtmfDecoderInit(&inst->DTMF_inst, 8000, 560);
    if (ok != 0)
    {
        return ok;
    }
#endif
    inst->NoOfExpandCalls = 0;
    inst->current_Codec = -1;
    inst->current_Payload = -1;

    inst->millisecondsPerCall = 10;
    inst->timestampsPerCall = inst->millisecondsPerCall * 8;
    inst->fs = 8000;
    inst->first_packet = 1;

    WebRtcNetEQ_ResetMcuInCallStats(inst);

    WebRtcNetEQ_ResetWaitingTimeStats(inst);

    WebRtcNetEQ_ResetMcuJitterStat(inst);

    WebRtcNetEQ_ResetAutomode(&(inst->BufferStat_inst.Automode_inst),
        inst->PacketBuffer_inst.maxInsertPositions);

    return 0;
}





int WebRtcNetEQ_ResetMcuInCallStats(MCUInst_t *inst)
{
    inst->lostTS = 0;
    inst->lastReportTS = 0;
    inst->PacketBuffer_inst.discardedPackets = 0;

    return 0;
}





void WebRtcNetEQ_ResetWaitingTimeStats(MCUInst_t *inst) {
  memset(inst->waiting_times, 0,
         kLenWaitingTimes * sizeof(inst->waiting_times[0]));
  inst->len_waiting_times = 0;
  inst->next_waiting_time_index = 0;
}





void WebRtcNetEQ_StoreWaitingTime(MCUInst_t *inst, int waiting_time) {
  assert(inst->next_waiting_time_index < kLenWaitingTimes);
  inst->waiting_times[inst->next_waiting_time_index] = waiting_time;
  inst->next_waiting_time_index++;
  if (inst->next_waiting_time_index >= kLenWaitingTimes) {
    inst->next_waiting_time_index = 0;
  }
  if (inst->len_waiting_times < kLenWaitingTimes) {
    inst->len_waiting_times++;
  }
}





int WebRtcNetEQ_ResetMcuJitterStat(MCUInst_t *inst)
{
    inst->BufferStat_inst.Automode_inst.countIAT500ms = 0;
    inst->BufferStat_inst.Automode_inst.countIAT1000ms = 0;
    inst->BufferStat_inst.Automode_inst.countIAT2000ms = 0;
    inst->BufferStat_inst.Automode_inst.longestIATms = 0;

    return 0;
}

