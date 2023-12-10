#include "utils/jsonobj.h"

static void ICACHE_FLASH_ATTR setup(JSONObject *self, char *json);
static void ICACHE_FLASH_ATTR delete(JSONObject *self);
static int ICACHE_FLASH_ATTR getInt(JSONObject *self, const char *key);
static char * ICACHE_FLASH_ATTR getString(JSONObject *self, const char *key);

static int32_t ICACHE_FLASH_ATTR keyAt(char *src, const char *sub, int32_t *pNext);
static void ICACHE_FLASH_ATTR generateNext(const char *match, uint32_t matchLength, int32_t *pNext);

/**
 * @brief JSONObject构造函数
 * @param bufferSize 字符串结果缓冲区大小，getString结果长度受限于此
 * */
JSONObject * ICACHE_FLASH_ATTR newJSONObject(uint32_t bufferSize) {
    JSONObject *object = (JSONObject *)os_malloc(sizeof(JSONObject));
    object->workBuffer = (char *)os_malloc(sizeof(char) * bufferSize);
    object->setup = setup;
    object->getString = getString;
    object->getInt = getInt;
    object->delete = delete;
    return object;
}

/**
 * @brief 为json指定json格式字符串
 * @brief 由于*self仅引用*json，因此在json解析使用完毕之前，不要改变*json中的内容
 * @prarm *self json结构
 * @param *json json字符串
 * */
static void ICACHE_FLASH_ATTR setup(JSONObject *self, char *json) {
    if(json != NULL) {
        self->jsonStr = json;
    }
}

/**
 * @brief 从json字符串中提取string
 * @param *self json结构
 * @param *key json键
 * @return self->workBuffer：找到对应的value并复制进工作缓存，NULL：未找到对应的value
 * */
static char * ICACHE_FLASH_ATTR getString(JSONObject *self, const char *key) {
    int32_t pNext[JSON_KEY_MAX_LENGTH], find, total;
    uint32_t offset = 0, len = os_strlen(key);
    if(len > JSON_KEY_MAX_LENGTH) {
    	return NULL;
    }
    do {
        find = keyAt((self->jsonStr + offset), key, pNext);
        if(find == -1) {
            return NULL;
        }
        offset += find;
        if(*(self->jsonStr + offset - 1) == '\"' && *(self->jsonStr + offset + len) ==  '\"') {
            // 3 = strlen("\":\"")
            offset += (len + 3);
            break;
        }
        offset += (len + 1);
    }while(find != -1);
    // base position is 'offset'
    total = charAt((self->jsonStr + offset), '\"');
    if(total == -1) {
        return NULL;
    }
    // copy "value" to workbuffer
    os_memcpy(self->workBuffer, (self->jsonStr + offset), total);
    // add end tag
    *(self->workBuffer +  total) = '\0';
    return self->workBuffer;
}

/**
 * @brief 从json字符串中提取int
 * @param *self json结构
 * @param *key json键
 * @return key对应的value，找不到时默认返回0
 * */
static int ICACHE_FLASH_ATTR getInt(JSONObject *self, const char *key) {
    char tag;
    int32_t pNext[JSON_KEY_MAX_LENGTH], find;
    uint32_t offset = 0, len = os_strlen(key);
    if(len > JSON_KEY_MAX_LENGTH) {
    	return 0;
    }
    do {
        find = keyAt((self->jsonStr + offset), key, pNext);
        if(find == -1) {
            return 0;
        }
        offset += find;
        if(*(self->jsonStr + offset - 1) == '\"' && *(self->jsonStr + offset + len) ==  '\"') {
            // 2 = strlen("\":")
            offset += (len + 2);
            // 适配int也用双引号括起来的情况
            tag = *(self->jsonStr + offset);
            offset = (tag < '0' || tag > '9') ? (offset + 1) : offset;
            return atoi(self->jsonStr + offset);
        }
        offset += (len + 1);
    } while(find != -1);
    return 0;
}

/**
 * @brief JSONObject析构函数
 * */
static void ICACHE_FLASH_ATTR delete(JSONObject *self) {
    if(self != NULL && self->workBuffer != NULL) {
        os_free(self->workBuffer);
        os_free(self);
    }
}

/**
 * @brief KMP方式查找子串
 * @param *src 主串
 * @param *sub 子串
 * @param *pNext next匹配数组buffer(int32_t类型)，外部传入避免本函数内malloc，大小为子串长度
 * @return 子串在主串中首次出现的位置
 * */
static int32_t ICACHE_FLASH_ATTR keyAt(char *src, const char *sub, int32_t *pNext) {
    int32_t i = 0, j = 0;
    uint32_t srcLength, subLength;
    if(src == NULL || sub == NULL) {
        return -1;
    }
    srcLength = strlen(src);
    subLength = strlen(sub);
    //获得next数组
    generateNext(sub, subLength, pNext);

    while(i < srcLength && j < subLength) {
        if(src[i] == sub[j]) {
            //如果主串与子串所对应位置相等
            i++; j++;
        }else {
            //不相等则匹配（子）串跳转
            if(j == 0){
                i++;
            }else {
                j = pNext[j - 1];
            }
        }
    }
    // 返回子串在主串中首次出现的位置
    return (j == subLength) ? (i - j) : -1;
}

static void ICACHE_FLASH_ATTR generateNext(const char *match, uint32_t matchLength, int32_t *pNext) {
    int32_t i = 1, j = (i - 1);
    // 子串的第一个字符的Next一定为0
    pNext[0] = 0;
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

