/*
 * string_utils.c
 * @brief 字符串操作类，以单字节为基础，多字节编码需要自行处理
 * Created on: Aug 23, 2020
 * Author: Yanye
 */

#include "utils/strings.h"

static void ICACHE_FLASH_ATTR getNext(uint8_t *match, uint32_t matchLength, int32_t *pNext) ;

static uint8_t ICACHE_FLASH_ATTR byteStrTohex(uint8_t *str);

/**
 * @brief unicode格式字符串转gb2312编码, 末尾会补'\0'
 * @brief 可以接受纯unicode字符"\u591a\u4e91", 转换后变成小端模式0x1a 0x59 0x91 0x4e
 * @brief 可以接受ascii + unicode字符混合"3-4\u7ea7", 转换后ascii码不变，unicode变成小端模式0x33 0x2d 0x34 0xa7 0x7e
 * @param *unicode unicode字符串
 * @return 转换为gb2312编码的字符串, 实质上返回的char *同为输入的char *unicode, 因为转换后的数据会比输入数据小
 * */
uint8_t * ICACHE_FLASH_ATTR UnicodeToGB2312(uint8_t *unicode) {
    uint8_t *ch;
    Short code;
    uint32_t index = 0, write = 0;
    while(*(unicode + index) != 0x00) {
        ch = (unicode + index);
        // 仅处理unicode字符串，ascii部分保持不变
        if(*ch == '\\' && *(ch + 1) == 'u') {
        	// unicode 小端
        	code.bytes[0] = byteStrTohex(ch + 4);
        	code.bytes[1] = byteStrTohex(ch + 2);
        	code = QueryGB2312ByUnicode(code.value);
        	// gb2312大端
            *(unicode + write) = code.bytes[1];
            write += 1;
            *(unicode + write) = code.bytes[0];
            write += 1;
            // 跳过该unicode码
            index += 6;
            continue;
        }else {
            write++;
        }
        index++;
    }
    *(unicode + write) = '\0';
	return unicode;
}

/**
 * @brief 字节字符串形式转数值
 * @brief 字符串之前不加"0x"， 兼容大小写混合，例"30", "7C", "aB"
 * @param *str 字节字符串指针，所指字节字符串开头不加"0x"
 * @return value 转换后的字节数值
 * */
static uint8_t ICACHE_FLASH_ATTR byteStrTohex(uint8_t *str) {
    int i; uint8_t ch;
    uint8_t value = 0;
    for(i = 0; i < 2; i++) {
        ch = *(str + i);
        if(ch >= 0x30 && ch <= 0x39) {
            // 0 ~ 9
            value |= (ch - 0x30);
        }else if(ch >= 0x61 && ch <= 0x66) {
        	// a ~ f
            value |= (ch - 0x61 + 0xA);
        }else if(ch >= 0x41 && ch <= 0x46) {
        	// A ~ F
            value |= (ch - 0x41 + 0xA);
        }
        value <<= (4 - (i << 2));
    }
    return value;
}

/**
 * @brief 使用utf16.lut查找表查询unicode对应的gb2312编码
 * @brief unicode小端方式输入，gb2312大端方式输出
 * @param unicode 双字节unicode码, 小端模式
 * @return gb2312 gb2312字符集，大端模式
 * */
Short ICACHE_FLASH_ATTR QueryGB2312ByUnicode(uint16_t unicode) {
	File file;
	Short gb2312;
	BOOL success;
	uint16_t readin = 0;
	uint32_t pack, group;
	int32_t start, middle, end;
	gb2312.value = 0;

	if(open_file(&file, "utf16", "lut")) {
		// 四字节一组，低两字节为unicode，高两字节为gb2312
		group = (file.length / sizeof(uint32_t));
		middle = group / 2;
		end = group;
		start = -1;
		do {
			// 二分法，从中间查起
			success = read_file(&file, (middle * sizeof(uint32_t)), (uint8_t *)&pack, sizeof(uint32_t));
			if(!success || middle <= 0 || middle >= (group - 1)) {
				break;
			}
			readin = (uint16_t)(pack & 0xFFFF);
			if(unicode < readin) {
				// 向左查询
				end = middle;
				middle -= ((end - start) / 2);
			}else if(unicode > readin) {
				// 向右查询
				start = middle;
				middle += ((end - start) / 2);
			}
		}while(unicode != readin);

		gb2312.bytes[0] = (uint8_t)((pack >> 24) & 0xFF);
		gb2312.bytes[1] = (uint8_t)((pack >> 16) & 0xFF);
	}
	return gb2312;
}

/*
Unicode符号范围     |        UTF-8编码方式
(十六进制)        |              （二进制）
----------------------+---------------------------------------------
0000 0000-0000 007F | 0xxxxxxx
0000 0080-0000 07FF | 110xxxxx 10xxxxxx
0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
/**
 * @brief utf8编码转unicode字符集(usc4)，最大支持4字节utf8编码，(4字节以上在中日韩文中为生僻字)
 * @param *utf8 utf8变长编码字节集1~4个字节
 * @param *unicode utf8编码转unicode字符集结果，最大4个字节，返回的字节序与utf8编码序一致
 * @return length 0：utf8解码异常，others：本次utf8编码长度
 */
uint8_t ICACHE_FLASH_ATTR UTF8ToUnicode(uint8_t *utf8, uint32_t *unicode) {
    const uint8_t lut_size = 3;
    const uint8_t length_lut[] = {2, 3, 4};
    const uint8_t range_lut[] = {0xE0, 0xF0, 0xF8};
    const uint8_t mask_lut[] = {0x1F, 0x0F, 0x07};

    uint8_t length = 0;
    byte b = *(utf8 + 0);
    uint32_t i = 0;

    if(utf8 == NULL) {
        *unicode = 0;
        return PARSE_ERROR;
    }

    // utf8编码兼容ASCII编码,使用0xxxxxx 表示00~7F
    if(b < 0x80) {
        *unicode =  b;
        return 1;
    }
    // utf8不兼容ISO8859-1 ASCII拓展字符集
    // 同时最大支持编码6个字节即1111110X
    if(b < 0xC0 || b > 0xFD) {
        *unicode = 0;
        return PARSE_ERROR;
    }
    for(i = 0; i < lut_size; i++) {
        if(b < range_lut[i]) {
            *unicode = b & mask_lut[i];
            length = length_lut[i];
            break;
        }
    }
    // 超过四字节的utf8编码不进行解析
    if(length == 0) {
        *unicode = 0;
        return PARSE_ERROR;
    }
    // 取后续字节数据
    for(i = 1; i < length; i++ ) {
        b = *(utf8 + i);
        // 多字节utf8编码后续字节范围10xxxxxx‬~‭10111111‬
        if( b < 0x80 || b > 0xBF ) {
            break;
        }
        *unicode <<= 6;
        // ‭00111111‬
        *unicode |= (b & 0x3F);
    }
    // 长度校验
    return (i < length) ? PARSE_ERROR : length;
}

/**
 * @brief 4字节unicode(usc4)字符集转utf16编码
 * @param unicode unicode字符值
 * @param *utf16 utf16编码结果
 * @return utf16长度，(2字节)单位
 */
uint8_t ICACHE_FLASH_ATTR UnicodeToUTF16(uint32_t unicode, uint16_t *utf16) {
    // Unicode范围 U+000~U+FFFF
    // utf16编码方式：2 Byte存储，编码后等于Unicode值
    if(unicode <= 0xFFFF) {
    	unicode &= 0xFFFF;
    	os_memcpy(utf16, &unicode, sizeof(uint16_t));
		return 1;
	}else if( unicode <= 0xEFFFF ) {
		// 高10位
		*(utf16 + 0) = 0xD800 + (unicode >> 10) - 0x40;
		// 低10位
		*(utf16 + 1) = 0xDC00 + (unicode &0x03FF);
		return 2;
	}
    return 0;
}

/**
 * @brief 大小端交换
 * @param data 原始值
 * @param length 需要交换的字节长度
 * @return next.value 字节交换后的值
 * */
uint32_t ICACHE_FLASH_ATTR endianSwap(uint32_t data, uint32_t length) {
	Integer raw, next;
	uint32_t i = 0;
	raw.value = data;
    next.value = 0;
	while(length > 0) {
		next.bytes[i++] = raw.bytes[--length];
	}
	return next.value;
}

/**
 * @brief 查找字符串中某个字符的第一次出现位置，适用ASCII编码
 * @param *str 被查找的字符串
 * @param ch 目标字符
 * @return ch字符在*str中的位置，以0作为起点，不存在时返回-1
 */
int32_t ICACHE_FLASH_ATTR charAt(char *str, char ch) {
    int32_t position = 0;
    for(position = 0; ((*str != '\0') && (*str != ch)); str++, position++);
    return (*str == '\0') ? -1 : position;
}

/**
 * @brief 查找字符串中某个字符的第一次出现位置，并跳过该字符，适用ASCII编码
 * @param *str 被查找的字符串
 * @param ch 目标字符
 * @return 在*str中ch字符首次出现的下一个位置
 * */
int32_t ICACHE_FLASH_ATTR charAtSkip(char *str, char ch) {
	int32_t idx = charAt(str, ch);
	return (idx == -1) ? -1 : (idx + 1);
}

/**
 * @brief 从字符串尾部向前查找ch，返回ch所在的位置
 * */
int32_t ICACHE_FLASH_ATTR lastIndexOf(uint8_t *src, uint8_t ch) {
    int32_t position = os_strlen(src) - 1;
    for(; ((position > -1) && (*(src + position) != ch)); position--);
    return position;
}

/**
 * @brief 返回字符串下一行首地址 = (str + position)
 * @param *str 被处理的字符串，以\r\n(0D 0A)换行
 * @return position 基于str到下一行首字符的偏移量，到字符串末尾返回-1
 */
int32_t ICACHE_FLASH_ATTR nextLine(uint8_t *str) {
    uint32_t position = 0;
    // \r\n == 0D 0A
    while(*str != '\0') {
        if(*str == 0x0D) {
        	// 0D 0A换行
            position += 2;
            break;
        }else if(*str == 0x0A) {
        	// 0A 换行
        	position++;
        	break;
        }
        position++;
        str++;
    }
    return (*str == '\0') ? -1 : position;
}

/**
 * @brief 取str字符串的子串放入dest，用法与java的String.subString一致，子串范围为[from, to)
 * @param *str 源字符串
 * @param from 起始偏移量
 * @param to 结束偏移量
 * @param *dest 子串存储区
 * @return OK: 成功, FAIL: 失败
 */
STATUS ICACHE_FLASH_ATTR subString(uint8_t *str, uint32_t from, uint32_t to, uint8_t *dest) {
    if(to < from) {
        return FAIL;
    }
    if(str == NULL || dest == NULL) {
        return FAIL;
    }
    // @unsafe 仅适用于字符串边界外为\0字符且\0字符等于ASCII 0x00情形
    if((*(str + from) + *(str + to)) == '\0') {
        return FAIL;
    }
    // 使用memmove可以兼容str和dest地址有重叠的情形
    os_memmove(dest, (str + from), (to - from));
    // add end tag
    *(dest + to - from) = '\0';
    return OK;
}

/**
 * @brief 检测src字符串是否以prefix子串开头,uint8_t类型
 * */
BOOL ICACHE_FLASH_ATTR startsWith(uint8_t *src, uint8_t *prefix) {
    while(*prefix != '\0') {
        if(*src != *prefix) return FALSE;
        src++;
        prefix++;
    }
    return TRUE;
}

/**
 * @brief 检测src字符串是否以prefix子串开头, char类型
 * */
BOOL ICACHE_FLASH_ATTR startsWithStr(char *src, const char *prefix) {
    while(*prefix != '\0') {
        if(*src != *prefix) return FALSE;
        src++;
        prefix++;
    }
    return TRUE;
}

BOOL ICACHE_FLASH_ATTR startsWithStrIgnoreCase(char *src, const char *prefix) {
	char ps, pf;
    while(*prefix != '\0') {
    	// 大写全部转换成小写，小写不变
    	ps = *src | 0x20;
    	pf = *prefix | 0x20;
        if(ps != pf) return FALSE;
        src++;
        prefix++;
    }
    return TRUE;
}

/**
 * @brief 检测src字符串是否以prefix子串开头，其中src字符串忽略开头的空格与tab
 * @param *src 被查找字符串
 * @param *prefix 用于查找的子串
 * @return position: -1: 未找到,其它prefix子串在src字符串的开始位置
 */
int32_t ICACHE_FLASH_ATTR startWithEx(uint8_t *src, uint8_t *prefix) {
    uint32_t position = 0;
    while((*src == ' ') || (*src == '\t')) {
        src++;
        position++;
    }
    return (startsWith(src, prefix) == TRUE) ? position : -1;
}

/**
 * @brief 比较字符串，text长度必须短于src
 * */
BOOL ICACHE_FLASH_ATTR compareWith(char *src, const char *text) {
	while(*text != '\0') {
		if(*src != *text) {
			return FALSE;
		}
		src++; text++;
	}
	return TRUE;
}

BOOL ICACHE_FLASH_ATTR compareWithEx(char *src, uint8_t *target) {
	while(*src != '\0') {
		if(*src != *target) {
			return FALSE;
		}
		src++; target++;
	}
	return TRUE;
}

/**
 * @brief 替换src字符串中searchChar字符为newChar
 * @param *src 被替换的字符串
 * @param length src字符串的长度
 * @param searchChar 查找的字符
 * @param newChar 用作替换的字符
 * @return 成功替换的字符个数
 * */
uint32_t ICACHE_FLASH_ATTR replaceWith(char *src, uint32_t length, char searchChar, char newChar) {
	uint32_t replaced = 0, i;
	for(i = 0; i < length; i++) {
		if(*(src + i) == searchChar) {
			*(src + i) = newChar;
			replaced++;
		}
	}
	return replaced;
}

/**
 * @brief 对src去除头尾空格/tab 空白符
 * @param *src 源字符串
 * @return STATUS FAIL: 失败, OK: 成功
 */
STATUS ICACHE_FLASH_ATTR trim(uint8_t *src) {
    uint32_t start = 0, end = 0;
    if(src == NULL) {
        return FAIL;
    }
    while(*(src + start) != '\0') {
        // ' ' == 0x20, '\t' == 0x09
        if((*(src + start) != 0x20) && (*(src + start) != 0x09)) {
            break;
        }
        start++;
    }
    end = start;
    while(*(src + end) != '\0') {
    	end++;
    }
    end--;

    while(end > start) {
        if((*(src + end) != 0x20) && (*(src + end) != 0x09)) {
            break;
        }
        end--;
    }

    if(end > start) {
        os_memmove(src, (src + start), (end - start + 1));
        *(src + (end - start) + 1) = '\0';
        return OK;
    }
    return FAIL;
}

/**
 * @brief 判断src字符串中子串sub第一次出现的位置
 * @param *src 被查找字符串
 * @param *sub 用于查找的字串 长度有KMP_SUB_MAX_LENGTH限制，如果使用malloc则取决于可用内存
 * @return position >=0: 字串在主串首次出现位置，-1: 未找到或产生异常
 */
int32_t ICACHE_FLASH_ATTR contains(uint8_t *src, uint8_t *sub) {
    int32_t i = 0, j = 0;
    uint32_t srclen, sublen;
    int32_t pNext[KMP_SUB_MAX_LENGTH];
    if(src == NULL || sub == NULL) {
        return -1;
    }
    if((sublen = os_strlen(sub)) > KMP_SUB_MAX_LENGTH) {
    	return -1;
    }
    srclen = os_strlen(src);
    // 获得next数组
    getNext(sub, sublen, pNext);

    while((i < srclen) && (j < sublen)) {
        if(src[i] == sub[j]) {
            //如果主串与子串所对应位置相等
            i++; j++;
        }else {
            //不相等则匹配（子）串跳转
            if(j == 0){
                i++;
            }else{
                j = pNext[j - 1];
            }
        }
    }
    // 返回子串在主串中首次出现的位置
    return (j == sublen) ? (i - j) : -1;
}

static void ICACHE_FLASH_ATTR getNext(uint8_t *match, uint32_t matchLength, int32_t *pNext) {
    //子串的第一个字符的Next一定为0
    pNext[0] = 0;
    int32_t i = 1, j = (i - 1);
    while(i < matchLength)  {
        if(match[i] == match[pNext[j]]) {
            //如果与前面的字符相等
            pNext[i] = pNext[j] + 1;
            i++;
            j = (i - 1);
        }else {
            //不相等
            if(pNext[j] == 0) {
                //前一个next值是0
                pNext[i] = 0;
                i++;
                j = (i - 1);
            }else {
                //前一个next值非0
                j = (pNext[j] - 1);
            }
        }
    }
}

/**
 * @brief 检查str字符串是否在max长度内以'\0'结尾
 * @param *str 字符串
 * @param max str的容量
 * @return max：没有找到\0结束符, 其他：返回值等同于strlen
 * */
uint32_t ICACHE_FLASH_ATTR strlenEx(char *str, uint32_t max) {
	uint32_t i = 0;
	for(; i < max; i++) {
		if(*(str + i) == '\0') {
			break;
		}
	}
	return i;
}

/**
 * @brief 整形值转换成字符串，如果是负数会在最前面加'-'号
 * @param value 被转换的值
 * @param buffer 用于存储转换结果的字符缓冲区，输出10进制结果
 * @paran length 字符缓冲区的容量(字节)，当value值超出bufferLength时直接返回-1
 * @return 转换后字符串有效的长度，不含末尾的'\0'，最大不超出(<=(bufferLength - 1))
 * */
int32_t ICACHE_FLASH_ATTR integer2String(int32_t value, uint8_t *buffer, uint32_t length) {
	const char *NUMBER_CHARSET = "0123456789";
	// value最大范围-2147483648~2147483647, 算上'\0'
	char temp[12];
	uint32_t flag, i, j, num;
	temp[0] = '-';
	i = flag = (uint32_t)((value >> 31) & 0x1);
    // 负数变正数, num需要为unsigned类型
    // 因为-2147483648转换成+2147483648超出了int32_t上限(2147483647)
    num = flag ? ((~value) + flag) : value;
    do {
        temp[i] = NUMBER_CHARSET[num % 10];
        num /= 10;
        i++;
    }while(num != 0);
    // 算上末尾需要补的'\0'
    if((i + 1) > length) {
        return -1;
    }
    // 反向拷贝temp到buffer
    buffer[0] = '-';
    buffer[i] = '\0';
    for(j = flag; j < i; j++) {
        buffer[j] = temp[i - j - 1 + flag];
    }
    // 返回字符串长度，不含末尾的'\0'
    return i;
}
