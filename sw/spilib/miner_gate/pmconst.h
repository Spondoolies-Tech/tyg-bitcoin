#ifndef __PM_CONSTANTS_H_
#define __PM_CONSTANTS_H_

#define PM_FAULT_REG 0x79

#define PM_FAULT_VOUT 			((1 << 7) << 8)
#define PM_FAULT_LOUTPOUT		((1 << 6) << 8)
#define PM_FAULT_INPUT 			((1 << 5) << 8)
#define PM_FAULT_MFR			((1 << 4) << 8)
#define PM_FAULT_POWERGOOD		((1 << 3) << 8)
#define PM_FAULT_FAN			((1 << 2) << 8)
#define PM_FAULT_OTHER 			((1 << 1) << 8)
#define PM_FAULT_UNKNOWN 		((1 << 0) << 8)

#define PM_FAULT_BUSY			(1 << 7) 
#define PM_FAULT_OFF			(1 << 6) 
#define PM_FAULT_OV 			(1 << 5) 
#define PM_FAULT_OC 			(1 << 4) 
#define PM_FAULT_UV 			(1 << 3) 
#define PM_FAULT_TEMP 			(1 << 2) 
#define PM_FAULT_CML 			(1 << 1) 
#define PM_FAULT_NONE 			(1 << 0) 

#define PM_VOUT_STATUS	0x7A

#define VOUT_OV_FLT	(1 << 7) 
#define VOUT_OV_WRN	(1 << 6) 
#define VOUT_UV_WRN	(1 << 5) 
#define VOUT_UV_FLT	(1 << 4) 
#define VOUT_MAX_WRN (1 << 3) 
#define TON_MAX_WRN	(1 << 2) 
#define TOFF_MAX_WRN (1 << 1) 
#define PWR_ON_TRK_ERR (1 << 0) 


#define PM_IOUT_STATUS	0x7B

#define IOUT_OC_FLT	(1 << 7) 
#define OVC_FLT_LV_SHTDWN	(1 << 6) 
#define IOUT_OC_WRN	(1 << 5) 
#define IOUT_UC_FLT	(1 << 4) 
#define CURR_SHARE_FLT (1 << 3) 
#define IN_PWR_LIM_MODE	(1 << 2) 
#define POUT_OP_FLT (1 << 1) 
#define POUT_OP_WRN (1 << 0) 


#define PM_VIN_STATUS	0x7C

#define VIN_OV_FLT	(1 << 7) 
#define VIN_OV_WRN	(1 << 6) 
#define VIN_UV_WRN	(1 << 5) 
#define VIN_UV_FLT	(1 << 4) 
#define U_OFF_LOW_VIN (1 << 3) 
#define VIN_OC_FLT	(1 << 2) 
#define VIN_OC_WRN (1 << 1) 
#define PIN_OP_WRN (1 << 0) 


#define PM_TEMP_STATUS	0x7D

#define TEMP_OT_FLT	(1 << 7) 
#define TEMP_OT_WRN	(1 << 6) 
#define TEMP_UT_WRN	(1 << 5) 
#define TEMP_UT_FLT	(1 << 4) 
// BIT 0-3 in 0x7D Reserved


#define PM_CML_STATUS	0x7E

#define CML_INV_CMD	(1 << 7) 
#define CML_INV_DATE (1 << 6) 
#define PCK_ERR_CHK_FLT	(1 << 5) 
#define MEM_FLT_DETECTED	(1 << 4) 
#define PROC_FLT_DETECTED (1 << 3) 
//#define Reserved	(1 << 2) 
#define OTHER_COMM_FLT (1 << 1) 
#define OTHER_MEM_FLT (1 << 0) 


#define PM_FAN_STATUS	0x81

#define FAN1_FLT	(1 << 7) 
#define FAN2_FLT	(1 << 6) 
#define FAN1_WRN	(1 << 5) 
#define FAN2_FLT	(1 << 4) 
#define FAN1_SPD_OVRDN (1 << 3) 
#define FAN2_SPD_OVRDN	(1 << 2) 
#define AIRFLOW_FLT (1 << 1) 
#define AIRFLOW_WRN (1 << 0) 

#define PM_READ_VIN	0x88
#define PM_READ_IIN	0x89
#define PM_READ_VCAP	0x8A
#define PM_READ_VOUT	0x8B
#define PM_READ_IOUT	0x8C
#define PM_READ_TEMP1	0x8D
#define PM_READ_TEMP2	0x8E
#define PM_READ_TEMP3	0x8F
#define PM_READ_FAN_SPEED1	0x90
#define PM_READ_POUT 0x96
#define PM_READ_PIN	0x97

#endif
