























#ifndef ARM_EX_TABLES_H
#define ARM_EX_TABLES_H

typedef enum arm_exbuf_cmd {
  ARM_EXIDX_CMD_FINISH,
  ARM_EXIDX_CMD_DATA_PUSH,
  ARM_EXIDX_CMD_DATA_POP,
  ARM_EXIDX_CMD_REG_POP,
  ARM_EXIDX_CMD_REG_TO_SP,
  ARM_EXIDX_CMD_VFP_POP,
  ARM_EXIDX_CMD_WREG_POP,
  ARM_EXIDX_CMD_WCGR_POP,
  ARM_EXIDX_CMD_RESERVED,
  ARM_EXIDX_CMD_REFUSED,
} arm_exbuf_cmd_t;

struct arm_exbuf_data
{
  arm_exbuf_cmd_t cmd;
  uint32_t data;
};

#define arm_exidx_extract	UNW_OBJ(arm_exidx_extract)
#define arm_exidx_decode	UNW_OBJ(arm_exidx_decode)
#define arm_exidx_apply_cmd	UNW_OBJ(arm_exidx_apply_cmd)

int arm_exidx_extract (struct dwarf_cursor *c, uint8_t *buf);
int arm_exidx_decode (const uint8_t *buf, uint8_t len, struct dwarf_cursor *c);
int arm_exidx_apply_cmd (struct arm_exbuf_data *edata, struct dwarf_cursor *c);

#endif 
