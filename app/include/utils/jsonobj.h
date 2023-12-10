#ifndef _JSONOBJ_H_
#define _JSONOBJ_H_

#include "c_types.h"
#include "mem.h"
#include "osapi.h"
#include "strings.h"

// json键的最大长度(字节)
#define JSON_KEY_MAX_LENGTH    32
// getString返回值的最大长度(字节)
#define JSON_BUFFER_SIZE_DEFAULT    32

struct _jsonobject;
typedef struct _jsonobject JSONObject;

struct _jsonobject {
    char *jsonStr;
    // 用于存放getString返回数据，避免频繁malloc/free
    char *workBuffer;
    void (*setup)(JSONObject *self, char *json);
    char * (*getString)(JSONObject *self, const char *key);
    int (*getInt)(JSONObject *self, const char *key);
    void (*delete)(JSONObject *self);
};

JSONObject * ICACHE_FLASH_ATTR newJSONObject(uint32_t bufferSize);

#endif // JSON_H_INCLUDED
