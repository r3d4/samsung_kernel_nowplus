
/*
 * Parameter Infomation
 */

#ifndef ASM_MACH_PARAM_H
#define ASM_MACH_PARAM_H


#define SPP(volume)		(vs[volume].nSctsPerPg)
#define PARAM_LEN		(128 * 1024) 

#define PARAM_MAGIC			0x72726624
#define PARAM_VERSION			0x13	/* Rev 1.3 */

#define COMMAND_LINE_SIZE		1024
#define PARAM_STRING_SIZE		1024	/* 1024 Characters */
#define PARAM_VERSION_LENGTH		16	/* 1024 Characters */

typedef struct _param_int_t {
	param_idx ident;
	s32  value;
} param_int_t;


typedef struct _param_str_t {
	param_idx ident;
	s8 value[PARAM_STRING_SIZE];
} param_str_t;

typedef struct _param_union_t {
	param_int_t param_int_file1[COUNT_PARAM_FILE1];
	param_int_t param_int_file2[COUNT_PARAM_FILE2 - MAX_STRING_PARAM]; 
}param_union_t;


typedef struct {
	int param_magic;
	int param_version;
	union _param_int_list{	   
		param_int_t param_list[MAX_PARAM - MAX_STRING_PARAM];
		param_union_t param_union;
	} param_int_list;
	param_str_t param_str_list[MAX_STRING_PARAM];
} status_t;

typedef struct {
	int param_magic;
	int param_version; 
	param_int_t param_int_file1[COUNT_PARAM_FILE1]; 
}param_file1_t;  


typedef struct {
	param_int_t param_int_file2[COUNT_PARAM_FILE2 - MAX_STRING_PARAM]; 
	param_str_t param_str_list[MAX_STRING_PARAM];
}param_file2_t;

#define param_size(type)	((sizeof(u32) * 2 + type) >> 2)
#define START_MAGIC		0x54410001
#define CMDLINE_MAGIC		0x54410009
#define END_MAGIC		0x00000000

//minwoo7945-lfs 
#define BOOT_PARAM_FILE_NAME1 "param1.boot"
#define BOOT_PARAM_FILE_NAME2 "param2.boot"

#define DEFAULT_DELTA_LOCATION	"/mnt/rsv"
#define TEMP_DELTA_LOCATION	"/usr/data2"

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);

#define USB_SEL_MASK	(1 << 0)
#define UART_SEL_MASK	(1 << 1)


#endif	/* ASM_MACH_PARAM_H */
