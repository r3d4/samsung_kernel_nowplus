/*
 * modules/camera/cam_pmic.h
 *
 * Camera PMIC driver header file
 *
 * Written by paladin in Samsung Electronics
 */

#ifndef CAM_PMIC_H
#define CAM_PMIC_H

#define CAM_PMIC_DBG          0
#define CAM_PMIC_DRIVER_NAME  "cam_pmic"
#define CAM_PMIC_MOD_NAME     "CAM_PMIC: "
#define CAM_PMIC_I2C_RETRY    10

#define CAM_PMIC_I2C_ADDR  0x7D

#define GENERAL_SETTINGS    0x00
#define LDO1_SETTINGS       0x01
#define LDO2_SETTINGS       0x02
#define LDO3_SETTINGS       0x03
#define LDO4_SETTINGS       0x04
#define LDO5_SETTINGS       0x05
#define BUCK_SETTINGS1      0x06
#define BUCK_SETTINGS2      0x07
#define ENABLE_BITS         0x08      

#define LDO1_V_2V8          0x19
#define LDO2_V_2V8          0x19
#define LDO3_V_1V8          0x0c
#define LDO4_V_1V8          0x11
#define LDO5_V_2V8          0x19
#define BUCK_V1_1V2         0x09

#define LDO1_T_DEFAULT      0x7
#define LDO2_T_DEFAULT      0x7
#define LDO3_T_DEFAULT      0x3
#define LDO4_T_DEFAULT      0x7
#define LDO5_T_DEFAULT      0x7
#define BUCK_T1_DEFAULT     0x7

#define ENABLE_ALL          0x3f
#define DVS_V1              (1<<7)


struct cam_pmic {
  struct i2c_client *i2c_client;
};

#define CAM_PMIC_UNINIT_VALUE   0xFF

int cam_pmic_read_reg(u8 reg, u8* val);
int cam_pmic_write_reg(u8 reg, u8 val);
#endif /* ifndef CAM_PMIC_H */
