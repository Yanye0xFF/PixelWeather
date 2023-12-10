/*
 * udp_handler.c
 * @brief udp包处理程序
 * Created on: Sep 6, 2020
 * Author: Yanye
 */
#include "network/udp_handler.h"
#include "utils/fixed_file.h"

static ConnectionInfo connInfo;
static uint8_t *buffer = NULL;
static SparseArray *sparseArray = NULL;

static FileOpStatus fileOpState = FILE_OP_INIT;
static File currentFile;
static uint32_t totalSize;

static const uint8_t *deviceName = DEFAULT_NAME_PREFIX;

static os_timer_t udpTimer;
static uint32_t udpEventId;
struct espconn *espcbak;

static void ICACHE_FLASH_ATTR udpTimerCallback(void *timer_arg);
static uint32_t ICACHE_FLASH_ATTR macToString(uint8_t *dest, uint8_t *macBytes, uint32_t nBytes);
static STATUS ICACHE_FLASH_ATTR checkTokenAlive(uint32_t token);


static ICACHE_FLASH_ATTR void onBroadcastReceive(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onConnectRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onDisConnectRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onPowerModeRequest(struct espconn *espc, uint8_t *data, uint32_t length);

static ICACHE_FLASH_ATTR void onConfigWriteRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onConfigUpdateRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onConfigReadRequest(struct espconn *espc, uint8_t *data, uint32_t length);

static ICACHE_FLASH_ATTR void onRebootRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onNetUpdateRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onRuntimeModifyRequest(struct espconn *espc, uint8_t *data, uint32_t length);

static ICACHE_FLASH_ATTR void onFileUpInfoRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFileUpDataRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFileDownInfoRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFileDownDataRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFileRenameRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFileDeleteRequest(struct espconn *espc, uint8_t *data, uint32_t length);

static ICACHE_FLASH_ATTR void onFsListRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFsFormatRequest(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void onFsStatusRequest(struct espconn *espc, uint8_t *data, uint32_t length);

static ICACHE_FLASH_ATTR void displayBufferRead(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void sensorRead(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void deviceInfoRead(struct espconn *espc, uint8_t *data, uint32_t length);
static ICACHE_FLASH_ATTR void freezeFrame(struct espconn *espc, uint8_t *data, uint32_t length);

void ICACHE_FLASH_ATTR handlerInit() {
	if(buffer == NULL) {
		buffer = (uint8_t *)os_malloc(sizeof(uint8_t) * UDP_MAX_SIZE);
	}
	if(sparseArray == NULL) {
		sparseArray = newSparseArray(24);
	}
	os_timer_disarm(&udpTimer);
	os_timer_setfn(&udpTimer, &udpTimerCallback, NULL);
	// 清空connInfo
	memset(&connInfo, 0x00, sizeof(ConnectionInfo));
	sparseArray->clear(sparseArray);
	// 注册网络服务接口函数
	sparseArray->put(sparseArray, (size_t)BroadCastPackage, (size_t)onBroadcastReceive);
	sparseArray->put(sparseArray, (size_t)ConnectPackage, (size_t)onConnectRequest);
	sparseArray->put(sparseArray, (size_t)DisConnectPackage, (size_t)onDisConnectRequest);
	sparseArray->put(sparseArray, (size_t)PowerModePackage, (size_t)onPowerModeRequest);

	sparseArray->put(sparseArray, (size_t)ConfigWritePackage, (size_t)onConfigWriteRequest);
	sparseArray->put(sparseArray, (size_t)ConfigUpdatePackage, (size_t)onConfigUpdateRequest);
	sparseArray->put(sparseArray, (size_t)RebootPackage, (size_t)onRebootRequest);
	sparseArray->put(sparseArray, (size_t)NetUpdatePackage, (size_t)onNetUpdateRequest);
	sparseArray->put(sparseArray, (size_t)RuntimeModifyPackage, (size_t)onRuntimeModifyRequest);
	sparseArray->put(sparseArray, (size_t)ConfigReadPackage, (size_t)onConfigReadRequest);

	sparseArray->put(sparseArray, (size_t)FileUpInfoPackage, (size_t)onFileUpInfoRequest);
	sparseArray->put(sparseArray, (size_t)FileUpDataPackage, (size_t)onFileUpDataRequest);
	sparseArray->put(sparseArray, (size_t)FileDownInfoPackage, (size_t)onFileDownInfoRequest);
	sparseArray->put(sparseArray, (size_t)FileDownDataPackage, (size_t)onFileDownDataRequest);
	sparseArray->put(sparseArray, (size_t)FileRenamePackage, (size_t)onFileRenameRequest);
	sparseArray->put(sparseArray, (size_t)FileDeletePackage, (size_t)onFileDeleteRequest);

	sparseArray->put(sparseArray, (size_t)FsListPackage, (size_t)onFsListRequest);
	sparseArray->put(sparseArray, (size_t)FsFormatPackage, (size_t)onFsFormatRequest);
	sparseArray->put(sparseArray, (size_t)FsStatusPackage, (size_t)onFsStatusRequest);

	sparseArray->put(sparseArray, (size_t)DisplayBufferReadPackage, (size_t)displayBufferRead);
	sparseArray->put(sparseArray, (size_t)SensorReadPackage, (size_t)sensorRead);
	sparseArray->put(sparseArray, (size_t)DeviceInfoReadPackage, (size_t)deviceInfoRead);
	sparseArray->put(sparseArray, (size_t)FreezeFramePackage, (size_t)freezeFrame);
}

void ICACHE_FLASH_ATTR handlerDelete() {
	if(buffer != NULL) {
		os_free(buffer);
		buffer = NULL;
	}
	if(sparseArray != NULL) {
		sparseArray->delete(&sparseArray);
	}
	// 删除连接信息
	memset(&connInfo, 0x00, sizeof(ConnectionInfo));
}

void ICACHE_FLASH_ATTR handlerDispatch(struct espconn *espc, uint8_t *data, uint32_t length) {

#ifdef UDP_HANDLER_DEBUG
	uint32_t len = 0;
	os_printf("handlerDispatch recv:\n");
	for(; len < 5; len++) {
		os_printf("0x%02x ", *(data + len));
	}
	os_printf("\n");
#endif

	BOOL result;
	size_t key, entrypoint;

	if(length == 0) return;

	espcbak = espc;
	key = *(data + 0);

	result = sparseArray->get(sparseArray, key, &entrypoint);
	if(result) {
		((void (*)(struct espconn *, uint8_t *, uint32_t))entrypoint)(espc, data, length);
	}
}

static ICACHE_FLASH_ATTR void onBroadcastReceive(struct espconn *espc, uint8_t *data, uint32_t length) {
	struct ip_info info;
	uint16_t localPort = UDP_LISTEN_PORT, cursor = 0;
	uint8_t localMac[MAC_ADDR_SIZE];
	sint8_t wifiRssi, ifIndex;
	SystemConfig *config = NULL;

	ifIndex = wifi_get_opmode();
	if(STATION_MODE == ifIndex) {
		ifIndex = STATION_IF;
		wifiRssi = wifi_station_get_rssi();
	}else if(SOFTAP_MODE == ifIndex) {
		ifIndex = SOFTAP_IF;
		wifiRssi = 0;
	}else {
		// 其他模式下不响应广播
		return;
	}

	os_memcpy((espc->proto.udp->remote_ip), (data + HEADER_LENGTH), sizeof(uint32_t));
	os_memcpy(&(espc->proto.udp->remote_port), (data + HEADER_LENGTH + sizeof(uint32_t)), sizeof(uint16_t));

	config = sys_config_get();
	wifi_get_ip_info(ifIndex, &info);
	wifi_get_macaddr(ifIndex, localMac);

	*(buffer + cursor) = BroadCastPackage & 0xFF;
	cursor += 1;

	os_memcpy((buffer + cursor), &(info.ip.addr), sizeof(uint32_t));
	cursor += sizeof(uint32_t);

	os_memcpy((buffer + cursor), &localPort, sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	os_memcpy((buffer + cursor), localMac, MAC_ADDR_SIZE);
	cursor += MAC_ADDR_SIZE;

	os_memset((buffer + cursor), wifiRssi, sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	os_memset((buffer + cursor), HARDWARE_VERSION, sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	os_memset((buffer + cursor), SOFTWARE_VERSION, sizeof(uint8_t));
	cursor += sizeof(uint8_t);

	if(config == NULL) {
		*(buffer + cursor) = CONFIG_SYSTEM_STATUS_DEFAULT;
	}else {
		os_memcpy((buffer + cursor), &(config->status), sizeof(uint8_t));
	}
	cursor += sizeof(uint8_t);

	if(config != NULL && config->status.customName == STATUS_VALID && config->deviceName[0] != 0xFF) {
		// localPort 复用deviceName长度
		localPort = (os_strlen((const char *)config->deviceName) + 1);
		os_memcpy((buffer + cursor), config->deviceName, localPort);
		cursor += localPort;
	}else {
		os_memcpy((buffer + cursor), deviceName, DEFAULT_NAME_LENGTH);
		cursor += DEFAULT_NAME_LENGTH;
		cursor += macToString((buffer + cursor), (localMac + 4), 2);
		*(buffer + cursor) = '\0';
		cursor += 1;
	}
#ifdef UDP_HANDLER_DEBUG
	os_printf("onBroadcastReceive output:\n");
	for(localPort = 0; localPort < cursor; localPort++) {
		os_printf("0x%02x ", *(buffer + localPort));
	}
	os_printf("\n");
#endif
	espconn_send(espc, buffer, cursor);
}

static ICACHE_FLASH_ATTR void onConnectRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t i = 0, len;
	uint8_t password[32];
	uint8_t pwdlen = *(data + 7);
	SystemConfig *config = NULL;

	config = sys_config_get();

	buffer[0] = ConnectPackage;

	if(pwdlen > PWD_LENGTH_MAX) {
		buffer[1] = PWD_LENGTH_WRONG;
		espconn_send(espc, buffer, 2);
		return;
	}

	if(pwdlen > 0) {
		os_memset(password, 0x00, 32);
		os_memcpy(password, (data + 8), pwdlen);
	}

	if(config != NULL && config->status.needPwd == STATUS_VALID) {
		len = os_strlen(config->accessPwd);
		if(len != pwdlen) {
			buffer[1] = PWD_LENGTH_WRONG;
			espconn_send(espc, buffer, 2);
			return;
		}
		if(!compareWithEx(config->accessPwd, password)) {
			buffer[1] = PWD_WRONG;
			espconn_send(espc, buffer, 2);
			return;
		}
	}
	// 不需要连接密码/验证通过
	os_memcpy(&(connInfo.hostIp), (data + HEADER_LENGTH), sizeof(uint32_t));
	os_memcpy(&(connInfo.hostPort), (data + HEADER_LENGTH + sizeof(uint32_t)), sizeof(uint16_t));
	connInfo.timeout = TOKEN_TIMEOUT_MAX;
	// 随机生成token
	connInfo.token = os_random();

	buffer[0] = ConnectPackage & 0xFF;
	buffer[1] = UDP_RESULT_SUCCESS;
	os_memcpy((buffer + 2), &connInfo.token, sizeof(uint32_t));
#ifdef UDP_HANDLER_DEBUG
	os_printf("onConnectRequest output:\n");
	for(; i < 6; i++) {
		os_printf("0x%02x ", buffer[i]);
	}
	os_printf("\n");
#endif
	espconn_send(espc, buffer, TOKEN_LENGTH + 2);
}

static ICACHE_FLASH_ATTR void onDisConnectRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	os_memcpy(&usertoken, (data + 1), sizeof(uint32_t));

	buffer[0] = DisConnectPackage;
	// 检查用户token
	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 清空connInfo
	memset(&connInfo, 0x00, sizeof(ConnectionInfo));
	buffer[1] = UDP_RESULT_SUCCESS;
	espconn_send(espc, buffer, 2);
}


static ICACHE_FLASH_ATTR void onPowerModeRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint8_t option, opmode;
	uint32_t temp;
	PowerMode mode;
	SystemRunTime *runtime;

	opmode = wifi_get_opmode();
	runtime = sys_runtime_get();

	option = *(data + 1);
	mode.type = *(data + 2);
	os_memcpy(&temp, (data + 3), sizeof(uint32_t));
	buffer[0] = PowerModePackage;

	do {
		// 检查用户token
		if(checkTokenAlive(temp) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			temp = 2;
			break;
		}

		if(option == POWERMODE_WRITE) {
			temp = 2;
			mode.refreshAfterConnect = FALSE;
			mode.wifiState = (mode.type == POWER_NONE_SLEEP) ? POWER_STATE_WIFI_DEFAULT : POWER_STATE_WIFI_ON;

			if(STATION_MODE == opmode && runtime->internet) {
				system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&mode, sizeof(PowerMode));
				if(POWER_SHUTDOWN == mode.type) {
					udpEventId = UDP_TIMER_EVENT_SHUTDOWN;
					os_timer_arm(&udpTimer, 500, FALSE);
				}
				buffer[1] = UDP_RESULT_SUCCESS;

			}else {
				// 未配置时的AP模式
				if(POWER_MODEM_SLEEP == mode.type || POWER_LIGHT_SLEEP == mode.type) {
					buffer[1] = POWERMODE_NOT_ALLOWED;
					break;
				}
				buffer[1] = UDP_RESULT_SUCCESS;
				system_rtc_mem_write(POWERMODE_INFO_POS, (const void *)&mode, sizeof(PowerMode));
				if(POWER_SHUTDOWN == mode.type) {
					udpEventId = UDP_TIMER_EVENT_SHUTDOWN;
					os_timer_arm(&udpTimer, 500, FALSE);
				}
			}

		}else {
			// 读取当前低功耗模式
			system_rtc_mem_read(POWERMODE_INFO_POS, (void *)&mode, sizeof(PowerMode));
			os_memcpy((buffer + 2), &mode, sizeof(PowerMode));
			buffer[1] = POWERMODE_READ;
			temp = 6;
		}

	}while(0);

	espconn_send(espc, buffer, temp);
}

/**
 * @brief 整体更新配置文件
 * */
static ICACHE_FLASH_ATTR void onConfigWriteRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint16_t crc;
	uint32_t usertoken;
	BOOL success;
	uint8_t retBuffer[2];
	fixed_file_t fixed_file;
	char *fileName;
	char *extName;

	retBuffer[0] = ConfigWritePackage;
	retBuffer[1] = UDP_RESULT_SUCCESS;

	do {
		// 检查用户token
		os_memcpy(&usertoken, (data + 21), sizeof(uint32_t));
		if(checkTokenAlive(usertoken) != OK) {
			retBuffer[1] = TOKEN_NOT_EXIST;
			break;
		}

		os_memcpy(&(fixed_file.section_size), (data + 13), sizeof(uint32_t));
		os_memcpy(&(fixed_file.max_size), (data + 17), sizeof(uint32_t));
		if(length < (fixed_file.section_size + 27)) {
			retBuffer[1] = CONFIG_WRITE_LENGTH_ERROR;
			break;
		}
		// 计算CRC
		os_memcpy(&crc, (data + 25), sizeof(uint16_t));
		if(crc16_ccitt((data + 27), fixed_file.section_size) != crc) {
			retBuffer[1] = CONFIG_WRITE_CRC_ERROR;
			break;
		}

		fileName = (char *)(data + 1);
		extName = (char *)(data + 9);
		if(fixed_file_open(&fixed_file, fileName, extName) == FALSE) {
			if(fixed_file_create(&fixed_file, fileName, extName) == FALSE) {
				retBuffer[1] = CONFIG_WRITE_CREATE_ERROR;
			}
		}
	}while(0);

	os_memcpy(buffer, (data + 27), fixed_file.section_size);

	if(retBuffer[1] == UDP_RESULT_SUCCESS) {
		success = fixed_file_append(&fixed_file, buffer, fixed_file.section_size);
		retBuffer[1] = success ? UDP_RESULT_SUCCESS : CONFIG_WRITE_WRITE_ERROR;
	}

	espconn_send(espc, retBuffer, 2);
}

static ICACHE_FLASH_ATTR void onConfigUpdateRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint32_t offset;
	uint16_t crc;
	fixed_file_t fixed_file;
	char *fileName;
	char *extName;

	os_memcpy(&usertoken, (data + 26), sizeof(uint32_t));

	buffer[0] = (uint8_t)ConfigUpdatePackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do{
		// 检查用户token
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		os_memcpy(&(fixed_file.section_size), (data + 13), sizeof(uint32_t));
		os_memcpy(&(fixed_file.max_size), (data + 17), sizeof(uint32_t));
		os_memcpy(&offset, (data + 21), sizeof(uint32_t));

		if(offset >= fixed_file.section_size) {
			buffer[1] = CONFIG_UPDATE_OUT_OF_RANGE;
			break;
		}
		// 计算CRC
		os_memcpy(&crc, (data + 30), sizeof(uint16_t));
		if(crc16_ccitt((data + 1), 29) != crc) {
			buffer[1] = CONFIG_UPDATE_CRC_ERROR;
			break;
		}

		fileName = (char *)(data + 1);
		extName = (char *)(data + 9);
		if(fixed_file_open(&fixed_file, fileName, extName) == FALSE) {
			buffer[1] = CONFIG_UPDATE_NOT_EXIST;
			break;
		}
		fixed_file_update(&fixed_file, offset, data[25]);
	} while(0);

	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onConfigReadRequest(struct espconn *espc, uint8_t *data, uint32_t lengt) {
	uint32_t usertoken;
	uint16_t crc;
	fixed_file_t fixed_file;
	char *fileName;
	char *extName;

	os_memcpy(&usertoken, (data + 21), sizeof(uint32_t));

	buffer[0] = (uint8_t)ConfigReadPackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do{
		// 检查用户token
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		os_memcpy(&(fixed_file.section_size), (data + 13), sizeof(uint32_t));
		os_memcpy(&(fixed_file.max_size), (data + 17), sizeof(uint32_t));

		fileName = (char *)(data + 1);
		extName = (char *)(data + 9);
		if(fixed_file_open(&fixed_file, fileName, extName) == FALSE) {
			buffer[1] = CONFIG_READ_NOT_EXIST;
			break;
		}
		// 读取配置文件内容
		if(fixed_file_read(&fixed_file, (buffer + 4), fixed_file.section_size) != fixed_file.section_size) {
			buffer[1] = CONFIG_READ_LENGTH_ERROR;
			break;
		}
	} while(0);

	if(buffer[1] == UDP_RESULT_SUCCESS) {
		// 读取成功, 计算数据区CRC
		crc = crc16_ccitt((buffer + 4), fixed_file.section_size);
		os_memcpy((buffer + 2), &crc, sizeof(uint16_t));
		espconn_send(espc, buffer, (fixed_file.section_size + 4));
	}else {
		espconn_send(espc, buffer, 2);
	}
}

static ICACHE_FLASH_ATTR void onRebootRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	os_memcpy(&usertoken, (data + 1), sizeof(uint32_t));
	buffer[0] = RebootPackage;

	do {
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		buffer[1] = UDP_RESULT_SUCCESS;
	}while(0);

	if(buffer[1] == UDP_RESULT_SUCCESS) {
		udpEventId = UDP_TIMER_EVENT_REBOOT;
		os_timer_arm(&udpTimer, 1500, FALSE);
	}
	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onNetUpdateRequest(struct espconn *espc, uint8_t *data, uint32_t length) {

	uint32_t time, current, elapse;
	SystemConfig *config;
	SystemRunTime *runtime;

	os_memcpy(&elapse, (data + 1), sizeof(uint32_t));
	buffer[0] = NetUpdatePackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do {
		if(checkTokenAlive(elapse) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}

		config = sys_config_get();
		if(config == NULL || (config->status.isConfiged == STATUS_INVALID) || (config->status.hasResource == STATUS_INVALID)) {
			buffer[1] = NETUPDATE_CONFIG_NOTEXIST;
			break;
		}
		runtime = sys_runtime_get();
		if(runtime->internet == 0) {
			buffer[1] = NETUPDATE_NO_INTERNET;
			break;
		}

		system_rtc_mem_read(NETUPDATE_POS, (void *)&time, sizeof(uint32_t));
		current = system_get_time();
		elapse = (current < time) ? (0xFFFFFFFF - time + current) : (current - time);
		// 至少间隔60s
		if(elapse < 60000000) {
			buffer[1] = NETUPDATE_TOO_OFTEN;
			break;
		}
	}while(0);

	if(buffer[1] == UDP_RESULT_SUCCESS) {
		// 先行写入当前时间防止多按
		system_rtc_mem_write(NETUPDATE_POS, (const void *)&current, sizeof(uint32_t));
		udpEventId = UDP_TIMER_EVENT_NETUPDATE;
		os_timer_arm(&udpTimer, 100, FALSE);
	}
	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onRuntimeModifyRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint8_t runtimeBit, runtimeValue;
	Result result;
	// 解析包数据
	os_memcpy(&usertoken, (data + 3), sizeof(uint32_t));
	runtimeBit = *(data + 1);
	runtimeValue = *(data + 2);

	buffer[0] = (uint8_t)RuntimeModifyPackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do{
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		if(runtimeBit > MAX_RUNTIME_BIT) {
			buffer[1] = CONFIG_BIT_OUTOF_RANGE;
			break;
		}
		if(runtimeValue > RUNTIME_VALUE_MAX) {
			buffer[1] = CONFIG_VALUE_WRONG;
			break;
		}
		sys_runtime_set(runtimeBit, runtimeValue);
	} while(0);

	espconn_send(espc, buffer, 2);
}

/**
 * @brief 文件接收，文件头部分响应
 * @param *data udp接口收到的数据
 * @param length udp数据的有效长度
 * */
static ICACHE_FLASH_ATTR void onFileUpInfoRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint8_t method;
	uint32_t usertoken;
	FileInfo finfo;
	uint16_t crc;
	// 重置状态机
	fileOpState = FILE_OP_INIT;
	// 解析包数据
	os_memcpy(&totalSize, (data + 13), sizeof(uint32_t));
	os_memcpy(&usertoken, (data + 21), sizeof(uint32_t));
	os_memcpy(&crc, (data + 26), sizeof(uint16_t));

	buffer[0] = FileUpInfoPackage;
	// 检查用户token
	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 检查文件索引区空间
	if(spifs_avail_files() < 1) {
		buffer[1] = FILE_NO_FILEBLOCK_SPACE;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 检查文件数据区空间
	if((spifs_avail_sector() * DATA_AREA_SIZE) < totalSize) {
		buffer[1] = FILE_NO_DATAAREA_SPACE;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 校验CRC
	// (FILENAME_FULLSIZE + 13) = (FILENAME_FULLSIZE(12bytes) + FileSize(4bytes) + FileInfo(4bytes) + Token(4bytes) + Method(1byte))
	if(crc16_ccitt((data + 1), (FILENAME_FULLSIZE + 13)) != crc) {
		buffer[1] = FILE_INIT_CRC_ERROR;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 重置0xFF
	os_memset(&currentFile, EMPTY_BYTE_VALUE, sizeof(File));
	// 解析包数据
	os_memcpy(currentFile.filename, (data + 1), FILENAME_SIZE);
	os_memcpy(currentFile.extname, (data + 9), EXTNAME_SIZE);
	os_memcpy(&finfo, (data + 17), sizeof(FileInfo));
	os_memcpy(&method, (data + 25), sizeof(uint8_t));
	// 根据写入方式处理，默认FILE_METHOD_NORMAL
	if(FILE_METHOD_OVERRIDE == method) {
		if(open_file_raw(&currentFile, currentFile.filename, currentFile.extname)) {
			delete_file(&currentFile);
		}
		buffer[1] = create_file(&currentFile, &finfo);
	}else if(FILE_METHOD_APPEND == method) {
		if(open_file_raw(&currentFile, currentFile.filename, currentFile.extname)) {
			buffer[1] = UDP_RESULT_SUCCESS;
		}else {
			buffer[1] = FILE_NOT_EXIST;
		}
	}else {
		// FILE_METHOD_NORMAL，create_file 返回码已和返回值类型匹配
		buffer[1] = create_file(&currentFile, &finfo);
	}
	// 允许追加文件内容
	fileOpState = FILE_OP_APPEND;
#ifdef UDP_HANDLER_DEBUG
	os_printf("onFileHeaderRequest retbuffer:\n");
	os_printf("0x%x 0x%x\n", buffer[0], buffer[1]);
#endif
	espconn_send(espc, buffer, 2);
}

/**
 * @brief 文件接收，文件内容部分响应，无缓存
 * @param *espc espconn descriptor
 * @param *data udp接口收到的数据
 * @param length udp数据的有效长度
 * */
static ICACHE_FLASH_ATTR void onFileUpDataRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	Result result;
	uint16_t crc, datalength, temp;
	// 解析包数据
	os_memcpy(&usertoken, (data + 1), sizeof(uint32_t));
	os_memcpy(&datalength, (data + 5), sizeof(uint16_t));
	os_memcpy(&crc, (data + 7), sizeof(uint16_t));
	//
	buffer[0] = (uint8_t)FileUpDataPackage;
	// 检查用户token
	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 文件状态
	if(fileOpState != FILE_OP_APPEND) {
		buffer[1] = FILE_OP_STATE_ERROR;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 长度超范围
	if(datalength > FILE_PAYLOAD_MAX) {
		buffer[1] = FILE_PAYLOAD_OUTOF_RANGE;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 校验CRC
	if(crc16_ccitt((data + 9), datalength) != crc) {
		buffer[1] = FILE_CONTENT_CRC_ERROR;
		espconn_send(espc, buffer, 2);
		return;
	}

	os_memcpy(buffer, (data + 9), datalength);
	// 追加写文件
	result = write_file(&currentFile, buffer, datalength, APPEND);

	buffer[0] = (uint8_t)FileUpDataPackage;
	buffer[1] = (result == APPEND_FILE_SUCCESS) ? UDP_RESULT_SUCCESS : result;
	// currentFile.length >= totalSize写入完成
	if(currentFile.length >= totalSize) {
		result = write_finish(&currentFile);
		buffer[1] = (result == APPEND_FILE_FINISH) ? FILE_WRITTEN_ACK : result;
		// 重置状态机
		fileOpState = FILE_OP_INIT;
	}
#ifdef UDP_HANDLER_DEBUG
	os_printf("onFileContentRequest retbuffer:\n");
	os_printf("0x%x 0x%x\n", buffer[0], buffer[1]);
#endif
	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onFileDownInfoRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	Result result;
	uint16_t crc;
	FileInfo finfo;
	uint8_t len = 2;

	// 解析包数据
	os_memcpy(currentFile.filename, (data + 1), FILENAME_SIZE);
	os_memcpy(currentFile.extname, (data + 9), EXTNAME_SIZE);
	os_memcpy(&usertoken, (data + 13), sizeof(uint32_t));
	os_memcpy(&crc, (data + 17), sizeof(uint16_t));

	fileOpState = FILE_OP_INIT;
	buffer[0] = (uint8_t)FileDownInfoPackage;
	do {
		// 检查用户token
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		// 校验CRC
		if(crc16_ccitt((data + 1), 16) != crc) {
			buffer[1] = FILE_DOWN_INFO_CRC_ERROR;
			break;
		}
		// 打开文件
		if(open_file_raw(&currentFile, currentFile.filename, currentFile.extname)) {
			read_finfo(&currentFile, &finfo);
			buffer[1] = UDP_RESULT_SUCCESS;
			os_memcpy((buffer + 2), &currentFile, sizeof(File));
			os_memcpy((buffer + 26), &finfo, sizeof(FileInfo));

			crc = crc16_ccitt((buffer + 2), 28);
			os_memcpy((buffer + 30), &crc, sizeof(uint16_t));
			len = 32;
			break;
		}
		buffer[1] = FILE_DOWN_NOT_EXIST;
	}while(0);

	fileOpState = FILE_OP_READ;
	espconn_send(espc, buffer, len);
}

static ICACHE_FLASH_ATTR void onFileDownDataRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken, offset, readlen = 0;
	uint16_t crc, len = 2;
	uint8_t mark;

	os_memcpy(&offset, (data + 1), sizeof(uint32_t));
	os_memcpy(&readlen, (data + 5), sizeof(uint16_t));
	mark = *(data + 7);
	os_memcpy(&usertoken, (data + 8), sizeof(uint32_t));
	os_memcpy(&crc, (data + 12), sizeof(uint16_t));

	buffer[0] = (uint8_t)FileDownDataPackage;
	do {
		if(fileOpState != FILE_OP_READ) {
			buffer[1] = FILE_DOWN_STATUS_ERROR;
			break;
		}
	    if(offset >= currentFile.length) {
			buffer[1] = FILE_DOWN_OFFSET_ERROR;
			break;
	    }
	    if((readlen == 0) || (readlen > FILE_PAYLOAD_MAX) || (readlen > currentFile.length)) {
			buffer[1] = FILE_DOWN_LENGTH_ERROR;
			break;
	    }
	    if(crc16_ccitt((data + 1), 11) != crc) {
	    	buffer[1] = FILE_DOWN_CRC_ERROR;
			break;
	    }
	    if((readlen = read_file(&currentFile, offset, (buffer + 8), readlen)) > 0) {
	    	buffer[1] = UDP_RESULT_SUCCESS;
	    	os_memcpy((buffer + 4), &readlen, sizeof(uint32_t));
	    	crc = crc16_ccitt((buffer + 4), (readlen + sizeof(uint32_t)));
	    	os_memcpy((buffer + 2), &crc, sizeof(uint16_t));
	    	len = (readlen + 8);
	    }else {
	    	buffer[1] = FILE_DOWN_LENGTH_ERROR;
	    }
	}while(0);

	if((len > 2) && (mark == 1)) {
		fileOpState = FILE_OP_INIT;
	}
#ifdef UDP_HANDLER_DEBUG
	os_printf("onFileDownDataRequest:");
	for(crc = 0; crc < 16; crc++) {
		os_printf("%02X ", buffer[crc]);
	}
	os_printf("\n");
#endif
	espconn_send(espc, buffer, len);
}

static ICACHE_FLASH_ATTR void onFileRenameRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint16_t crc;
	FileInfo finfo;
	Result result;
	uint8_t newfilename[FILENAME_SIZE];
	uint8_t newextname[EXTNAME_SIZE];

	os_memcpy(currentFile.filename, (data + 1), FILENAME_SIZE);
	os_memcpy(currentFile.extname, (data + 9), EXTNAME_SIZE);
	os_memcpy(newfilename, (data + 13), FILENAME_SIZE);
	os_memcpy(newextname, (data + 21), EXTNAME_SIZE);
	os_memcpy(&usertoken, (data + 25), sizeof(uint32_t));
	os_memcpy(&crc, (data + 29), sizeof(uint16_t));

	buffer[0] = (uint8_t)FileRenamePackage;
	do{
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		if(crc16_ccitt((data + 1), 28) != crc) {
			buffer[1] = RENAME_CRC_WRONG;
			break;
		}
		if(open_file_raw(&currentFile, currentFile.filename, currentFile.extname)) {
			read_finfo(&currentFile, &finfo);
			result = rename_file_raw(&currentFile, newfilename, newextname);
			buffer[1] = (FILE_RENAME_SUCCESS == result) ? UDP_RESULT_SUCCESS : RENAME_NO_PERMISSION;
		}else {
			buffer[1] = RENAME_FILE_NOT_EXIST;
		}
	}while(0);

#ifdef UDP_HANDLER_DEBUG
	os_printf("onFileRenameRequest:");
	for(crc = 0; crc < 2; crc++) {
		os_printf("%02X ", buffer[crc]);
	}
	os_printf("\n");
#endif
	espconn_send(espc, buffer, 2);
}

/**
 * @brief 删除文件请求
 * */
static ICACHE_FLASH_ATTR void onFileDeleteRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint16_t crc;
	uint8_t filename[FILENAME_SIZE];
	uint8_t extname[EXTNAME_SIZE];
	FileInfo finfo;

	os_memcpy(&usertoken, (data + 13), sizeof(uint32_t));
	os_memcpy(&crc, (data + 17), sizeof(uint16_t));

	buffer[0] = FileDeletePackage;
	// 检查用户token
	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 检查CRC
	if(crc16_ccitt((data + 1), (FILENAME_FULLSIZE + 4)) != crc) {
		buffer[1] = DELETE_CRC_WRONG;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 解析包数据
	os_memcpy(filename, (data + 1), FILENAME_SIZE);
	os_memcpy(extname, (data + 9), EXTNAME_SIZE);

	if(open_file_raw(&currentFile, filename, extname)) {
		read_finfo(&currentFile, &finfo);
		if(!(finfo.state.rw & finfo.state.del & finfo.state.dep)) {
			buffer[1] = DELETE_NO_PERMISSION;
			espconn_send(espc, buffer, 2);
			return;
		}
		delete_file(&currentFile);
		buffer[1] = DELETE_SUCCESS;
		espconn_send(espc, buffer, 2);
		return;
	}
	// 文件不存在
	buffer[1] = DELETE_FILE_NOT_EXIST;
	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onFsListRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken, size = 2, startAddr;
	uint16_t crc;
	uint8_t isIndex, loadSize;

#ifdef UDP_HANDLER_DEBUG

	os_printf("onFsListRequest: recv\n");
	for(crc = 0; crc < length; crc++) {
		os_printf("0x%02x ", *(data + crc));
	}
	os_printf("\n");

#endif

	os_memcpy(&isIndex, (data + 1), sizeof(uint8_t));
	os_memcpy(&loadSize, (data + 2), sizeof(uint8_t));
	os_memcpy(&usertoken, (data + 3), sizeof(uint32_t));
	os_memcpy(&crc, (data + 7), sizeof(uint16_t));

	buffer[0] = FsListPackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do {
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		if(loadSize > FS_LIST_LOAD_MAX) {
			buffer[1] = FS_LIST_LOAD_OUTOFRANGE;
			break;
		}
		// check crc
		if(crc16_ccitt((data + 1), 6) != crc) {
			buffer[1] = FS_LIST_CRC_WRONG;
			break;
		}
	}while(0);

	if(buffer[1] == UDP_RESULT_SUCCESS) {
		if(isIndex) {
			startAddr = (FB_SECTOR_START * SECTOR_SIZE);
		}else {
			system_rtc_mem_read(FILEBLOCK_START_POS, (void *)&startAddr, sizeof(uint32_t));
		}
		size = list_file_raw(&startAddr, (buffer + 4), loadSize);
		size = (size & 0xFFFF);
		system_rtc_mem_write(FILEBLOCK_START_POS, (const void *)&startAddr, sizeof(uint32_t));

		os_memcpy((buffer + 2), &size, sizeof(uint16_t));

		crc = crc16_ccitt((buffer + 1), (size * 28 + 3));

		startAddr = (size * 28 + 4);
		os_memcpy((buffer + startAddr), &crc, sizeof(uint16_t));

		size = (startAddr + sizeof(uint16_t));
	}

	espconn_send(espc, buffer, size);
}

static ICACHE_FLASH_ATTR void onFsFormatRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken, sectorStart, sectorEnd;
	uint16 temp;

	os_memcpy(&usertoken, (data + 9), sizeof(uint32_t));
	buffer[0] = FsFormatPackage;
	buffer[1] = UDP_RESULT_SUCCESS;

	do{
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		// 至少需要50%电量
		temp = system_adc_read();
		if(temp < 0x200) {
			buffer[1] = FS_FORMAT_LOW_POWER;
			break;
		}
	}while(0);

	if(buffer[1] == UDP_RESULT_SUCCESS) {
		// 先关闭时钟定时器
		EventBusGetDefault()->post(MAIN_EVENT_CLOSE_TIMER, 0);
		temp = TRUE;
		os_memcpy(&sectorStart, (data + 1), sizeof(uint32_t));
		os_memcpy(&sectorEnd, (data + 5), sizeof(uint32_t));
		for(; sectorStart <= sectorEnd && temp; sectorStart++) {
			temp = spifs_erase_sector(sectorStart);
		}
		buffer[1] = (temp) ? UDP_RESULT_SUCCESS : FS_FORMAT_OUTOF_RANGE;
	}

	espconn_send(espc, buffer, 2);
}

static ICACHE_FLASH_ATTR void onFsStatusRequest(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t cursor = 0, i;
	uint32_t temps[6];

	os_memcpy((temps + 0), (data + 1), sizeof(uint32_t));
	buffer[cursor] = FsStatusPackage;
	cursor += 1;

	if(checkTokenAlive(temps[0]) != OK) {
		buffer[cursor] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}

	buffer[cursor] = UDP_RESULT_SUCCESS;
	cursor += 1;

	temps[0] = FB_SECTOR_START;
	temps[1] = FB_SECTOR_END;
	temps[2] = DATA_SECTOR_START;
	temps[3] = DATA_SECTOR_END;
	temps[4] = spifs_avail_files();
	temps[5] = spifs_avail_sector();

	for(i = 0; i < 6; i++) {
		os_memcpy((buffer + cursor), (temps + i), sizeof(uint32_t));
		cursor += sizeof(uint32_t);
	}

	temps[0] = spifs_get_version();
	os_memcpy((buffer + cursor), (temps + 0), sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	temps[0] = crc16_ccitt((buffer + 1), (cursor - 1));
	os_memcpy((buffer + cursor), (temps + 0), sizeof(uint16_t));
	cursor += sizeof(uint16_t);

	espconn_send(espc, buffer, cursor);
}

static ICACHE_FLASH_ATTR void displayBufferRead(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken, cursor;
	uint16_t packLength;
	uint8_t readId;
	uint8_t *displayBuffer;

	readId = *(data + 1);
	os_memcpy(&usertoken, (data + 2), sizeof(uint32_t));

	buffer[0] = (uint8_t)DisplayBufferReadPackage;

	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}
	if(readId > DISPLAY_PACKAGE_ID_MAX) {
		buffer[1] = DISPLAY_PACKAGE_ID_OUTOF_RANGE;
		espconn_send(espc, buffer, 2);
		return;
	}

	displayBuffer = EPDGetDisplayRAM();
	buffer[1] = UDP_RESULT_SUCCESS;
	buffer[2] = readId;
	buffer[3] = (DISPLAY_PACKAGE_ID_MAX + 1);
	// 1072 = EPD_HEIGHT * EPD_RAM_WIDTH - DISPLAY_PACKAGE_ID_MAX * 2
	packLength = (readId < 2) ? DISPLAY_PACKAGE_LENGTH : (1072);
	os_memcpy((buffer+ 4), &packLength, sizeof(uint16_t));
	// copy display ram
	os_memcpy((buffer+ 8), (displayBuffer + DISPLAY_PACKAGE_LENGTH * readId), packLength);
	// 计算 crc, packLength复用做CRC结果
	packLength = crc16_ccitt((buffer + 8), packLength);
	os_memcpy((buffer+ 6), &packLength, sizeof(uint16_t));

	espconn_send(espc, buffer, UDP_MAX_SIZE);
}

/**
 * @brief 读取温湿度/电压传感器数据
 * */
static ICACHE_FLASH_ATTR void sensorRead(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	TempHumEntity *entity;
	uint16_t level;

	os_memcpy(&usertoken, (data + 1), sizeof(uint32_t));
	buffer[0] = (uint8_t)SensorReadPackage;

	if(checkTokenAlive(usertoken) != OK) {
		buffer[1] = TOKEN_NOT_EXIST;
		espconn_send(espc, buffer, 2);
		return;
	}

	DHT11Refresh();
	entity = DHT11Read();
	level = system_adc_read();
	buffer[1] = UDP_RESULT_SUCCESS;
	buffer[2] = entity->tempHighPart;
	buffer[3] = entity->humHighPart;
	os_memcpy((buffer + 4), &level, sizeof(uint16_t));
	espconn_send(espc, buffer, 6);
}

static ICACHE_FLASH_ATTR void deviceInfoRead(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint32_t len = 2;

	os_memcpy(&usertoken, (data + 1), sizeof(uint32_t));

	buffer[0] = (uint8_t)DeviceInfoReadPackage;
	buffer[1] = TOKEN_NOT_EXIST;

	do {
		if(checkTokenAlive(usertoken) != OK) {
			break;
		}
		buffer[1] = UDP_RESULT_SUCCESS;
		os_memset((buffer + 2), 0x00, 128);
		len = 130;
	}while(0);

	espconn_send(espc, buffer, len);
}

static ICACHE_FLASH_ATTR void freezeFrame(struct espconn *espc, uint8_t *data, uint32_t length) {
	uint32_t usertoken;
	uint16_t crc;
	FreezeFrame freezeFrame;

	os_memcpy(&usertoken, (data + 15), sizeof(uint32_t));
	os_memcpy(&crc, (data + 19), sizeof(uint16_t));

	buffer[0] = (uint8_t)FreezeFramePackage;

	do {
		if(checkTokenAlive(usertoken) != OK) {
			buffer[1] = TOKEN_NOT_EXIST;
			break;
		}
		if(crc != crc16_ccitt((data + 1), 18)) {
			buffer[1] = FREEZE_PACKAGE_CRC_ERROR;
			break;
		}
		// 文件名Byte3~10,拓展名Byte11~14
		if(!open_file_raw(&currentFile, (data + 3), (data + 11))) {
			buffer[1] = FREEZE_PACKAGE_FILE_NOT_EXIST;
			break;
		}
		// crc复用checkBMPFormat返回码
		if((crc = GuiCheckBMPFormat(&currentFile, NULL, NULL, NULL)) != 0) {
			buffer[1] = (crc == 1) ? FREEZE_PACKAGE_FILE_FORMAT_ERROR : FREEZE_PACKAGE_FILE_SIZE_OUTOF_RANGE;
			break;
		}
		buffer[1] = UDP_RESULT_SUCCESS;
		freezeFrame.type = (data[1] & 0x3);
		freezeFrame.align = data[2];
		system_rtc_mem_write(FREEZEFRAME_POS, (const void *)&freezeFrame, sizeof(FreezeFrame));
		fileOpState = FILE_OP_READ;
		udpEventId = UDP_TIMER_EVENT_FREEZE;
		os_timer_arm(&udpTimer, 1000, FALSE);
	}while(0);

	espconn_send(espc, buffer, 2);
}

static void ICACHE_FLASH_ATTR udpTimerCallback(void *timer_arg) {
	SystemRunTime *runtime;
	os_timer_disarm(&udpTimer);

	if(udpEventId == UDP_TIMER_EVENT_REBOOT) {
		runtime = sys_runtime_get();
		if(runtime->update) {
			EventBusGetDefault()->post(MAIN_EVENT_POWERTOGGLE, POWERTOGGLE_UPDATE);
		}else {
			system_restart();
		}
	}else if(udpEventId == UDP_TIMER_EVENT_NETUPDATE) {
		// 先关闭时钟定时器
		EventBusGetDefault()->post(MAIN_EVENT_CLOSE_TIMER, CLOSE_TIMER_AND_UPDATE);

	}else if(udpEventId == UDP_TIMER_EVENT_FREEZE && fileOpState == FILE_OP_READ) {
		fileOpState = FILE_OP_INIT;
		EventBusGetDefault()->post(MAIN_EVENT_DISPLAY_IMAGE, currentFile.block);

	}else if(UDP_TIMER_EVENT_SHUTDOWN == udpEventId) {
		EventBusGetDefault()->post(MAIN_EVENT_POWERTOGGLE, POWERTOGGLE_SHUTDOWN);
	}
}

/**
 * @brief 检查用户TOKEN是否有效，有效则刷新生命周期并返回OK，无效返回FAIL
 * @return STATUS FAIL：token无效或超时；OK：token有效
 * */
static STATUS ICACHE_FLASH_ATTR checkTokenAlive(uint32_t usertoken) {
	// 检查用户token
	if((usertoken != connInfo.token) || (connInfo.timeout == 0)) {
		return FAIL;
	}
	// 更新超时时间
	connInfo.timeout = TOKEN_TIMEOUT_MAX;
	return OK;
}

/**
 * @brief 刷新token超时时间，每分钟调用一次，单次登录token最长有效期255分钟
 * @note 只有在cpu运行时才刷新, 正常/modem_sleep
 * */
void ICACHE_FLASH_ATTR updateTokenTimeout(void) {
	if(connInfo.timeout > 0) {
		connInfo.timeout--;
	}
}

static const uint8_t *hexString = "0123456789ABCDEF";
static uint32_t ICACHE_FLASH_ATTR macToString(uint8_t *dest, uint8_t *macBytes, uint32_t nBytes) {
	uint32_t i;
	for(i = 0; i < nBytes; i++) {
		*(dest + (i << 1)) = *(hexString + (*(macBytes + i) >> 4 & 0xF));

		*(dest + (i << 1) + 1) = *(hexString + (*(macBytes + i) & 0xF));
	}
	return (nBytes << 1);
}
