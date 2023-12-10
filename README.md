# PixelWeather
a 2.13inch EPD weather station based on espressif ESP8266, use ESP8266_NONOS_SDK_v3.0.5.  

precompiled bin file also provided, check `pixelweather3.0/bin/` and use `flash_download_tools`.
| fileName  | address  | note  |
| ------------ | ------------ | ------------ |
| eagle.flash.bin  | 0x0  | must  |
| eagle.irom0text.bin  | 0x10000  | must  |
| esp_init_data_default_v05.bin  | 0x3fc000  | only first program |

Flash options，only support 4MB or above.  
![image](https://img2023.cnblogs.com/blog/1928719/202312/1928719-20231209212630858-1674974840.png)  

### Documents
https://www.cnblogs.com/yanye0xcc/p/14994806.html