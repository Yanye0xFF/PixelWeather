/*
 * udp_handler.h
 * @brief udp包处理程序
 * Created on: Sep 6, 2020
 * Author: Yanye
 */

#ifndef APP_USER_UDP_HANDLER_H_
#define APP_USER_UDP_HANDLER_H_

#include "c_types.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "spifsmini/spifs.h"

#include "network/crc16.h"

#include "driver/ssd1675b.h"
#include "driver/dht11.h"

#include "utils/sysconf.h"
#include "utils/sparse_array.h"
#include "utils/eventbus.h"
#include "utils/eventdef.h"
#include "utils/rtc_mem.h"

//#define UDP_HANDLER_DEBUG

/**
 * @brief 单用户+连接超时
 * */
typedef struct _connection_info{
	uint32_t token;
	uint32_t hostIp;
	uint16_t hostPort;
	uint8_t timeout;
	uint8_t connected;
} ConnectionInfo;

typedef enum _package_type{
	// 0x0x 基本配置类
	BroadCastPackage = 0x00,
	ConnectPackage = 0x01,
	DisConnectPackage = 0x02,
	PowerModePackage = 0x03,
	ConfigWritePackage = 0x04,
	ConfigUpdatePackage = 0x05,
	RebootPackage = 0x06,
	NetUpdatePackage = 0x07,
	RuntimeModifyPackage = 0x08,
	ConfigReadPackage = 0x09,

	// 0x1x 文件系列类
	FileUpInfoPackage = 0x10,
	FileUpDataPackage = 0x11,
	FileDownInfoPackage = 0x12,
	FileDownDataPackage = 0x13,
	FileRenamePackage = 0x14,
	FileDeletePackage = 0x15,
	FsListPackage = 0x16,
	FsFormatPackage = 0x17,
	FsStatusPackage = 0x19,

	// 0x2x 外设类
	DisplayBufferReadPackage = 0x20,
	SensorReadPackage = 0x21,
	DateReadPackage = 0x22,
	DeviceInfoReadPackage = 0x23,
	FreezeFramePackage = 0x24
} PackageType;

typedef enum _file_op_status {
	FILE_OP_INIT = 0,
	FILE_OP_APPEND = 1,
	FILE_OP_READ = 2
} FileOpStatus;

#define TOKEN_TIMEOUT_MAX        255

#define UDP_LISTEN_PORT          (8080)
#define UDP_MAX_SIZE             (1472)
#define MAC_ADDR_SIZE            (6)

// udp包头长度，固定一字节
#define HEADER_LENGTH            1

#define UDP_RESULT_SUCCESS     (0x00)
#define TOKEN_NOT_EXIST        (0xA0)

// @interface onConnectRequest
#define TOKEN_LENGTH           4
#define PWD_LENGTH_MAX         31
#define PWD_LENGTH_WRONG       (0x01)
#define PWD_WRONG              (0x02)

// @interface onNetUpdateRequest
#define NETUPDATE_CONFIG_NOTEXIST    0x01
#define NETUPDATE_NO_INTERNET        0x02
#define NETUPDATE_TOO_OFTEN          0x03

// @interface onPowerModeRequest
#define POWERMODE_NOT_ALLOWED       0x02
#define POWERMODE_WRITE             0x00
#define POWERMODE_READ              0x01

// @interface onFileHeaderRequest ref:create_file::returnVal
#define FILE_NO_FILEBLOCK_SPACE   0x1
#define FILE_NO_DATAAREA_SPACE    0x3
#define FILE_INIT_CRC_ERROR       0x4
#define FILE_NOT_EXIST            0x5
#define FILE_METHOD_NORMAL        (0x0)
#define FILE_METHOD_OVERRIDE      (0x1)
#define FILE_METHOD_APPEND        (0x2)

// @interface onFileContentRequest
#define FILE_OP_STATE_ERROR       0x8
#define FILE_PAYLOAD_OUTOF_RANGE  0x9
#define FILE_PAYLOAD_MAX          (1460)
#define FILE_WRITTEN_ACK          0xA
#define FILE_CONTENT_CRC_ERROR    0xB

// @interface onFileInfoRequest
#define FILE_DOWN_NOT_EXIST         (0x1)
#define FILE_DOWN_INFO_CRC_ERROR    (0x2)

// @interface onFileDownDataRequest
#define FILE_DOWN_OFFSET_ERROR    (0x1)
#define FILE_DOWN_LENGTH_ERROR    (0x2)
#define FILE_DOWN_CRC_ERROR       (0x3)
#define FILE_DOWN_STATUS_ERROR    (0x4)

// @interface onFileRenameRequest
#define RENAME_FILE_NOT_EXIST    0x01
#define RENAME_NO_PERMISSION     0x02
#define RENAME_CRC_WRONG         0x03

// @interface onFileDeleteRequest
#define DELETE_SUCCESS           0x00
#define DELETE_FILE_NOT_EXIST    0x01
#define DELETE_NO_PERMISSION     0x02
#define DELETE_CRC_WRONG         0x03

// @interface onFsListRequest
#define FS_LIST_CRC_WRONG          0x01
#define FS_LIST_LOAD_OUTOFRANGE    0x02
#define FS_LIST_LOAD_MAX           50

// @interface onFormatRequest
#define FS_FORMAT_LOW_POWER     (0x01)
#define FS_FORMAT_OUTOF_RANGE   (0x02)

// @interface onRuntimeModifyRequest
#define CONFIG_BIT_OUTOF_RANGE    (0x01)
#define CONFIG_VALUE_WRONG        (0x02)

// @interface onConfigUpdateRequest
#define CONFIG_UPDATE_NOT_EXIST     (0x01)
#define CONFIG_UPDATE_OUT_OF_RANGE  (0x02)
#define CONFIG_UPDATE_CRC_ERROR     (0x03)

// @interface onConfigWriteRequest
#define CONFIG_WRITE_LENGTH_ERROR     (0x01)
#define CONFIG_WRITE_CRC_ERROR        (0x02)
#define CONFIG_WRITE_CREATE_ERROR     (0x03)
#define CONFIG_WRITE_WRITE_ERROR      (0x04)

// @interface onConfigReadRequest
#define CONFIG_READ_NOT_EXIST         (0x01)
#define CONFIG_READ_LENGTH_ERROR      (0x02)

// @interface DisplayBufferRead
#define DISPLAY_PACKAGE_LENGTH            (1464)
#define DISPLAY_PACKAGE_ID_MAX            (2)
#define DISPLAY_PACKAGE_ID_OUTOF_RANGE    (0x01)

// @interface freezeFrame
#define FREEZE_PACKAGE_CRC_ERROR                (0x1)
#define FREEZE_PACKAGE_FILE_NOT_EXIST           (0x2)
#define FREEZE_PACKAGE_FILE_FORMAT_ERROR        (0x3)
#define FREEZE_PACKAGE_FILE_SIZE_OUTOF_RANGE    (0x4)

// @interface deviceInfoRead

#define UDP_TIMER_EVENT_REBOOT       100
#define UDP_TIMER_EVENT_NETUPDATE    101
#define UDP_TIMER_EVENT_FREEZE       102
#define UDP_TIMER_EVENT_SHUTDOWN     103

void ICACHE_FLASH_ATTR handlerInit();

void ICACHE_FLASH_ATTR handlerDelete();

void ICACHE_FLASH_ATTR handlerDispatch(struct espconn *espc, uint8_t *data, uint32_t length);

void ICACHE_FLASH_ATTR updateTokenTimeout(void);

#endif /* APP_USER_UDP_HANDLER_H_ */
