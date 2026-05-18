
idf_component_register(
  SRCS 
    "main.c"
    "smak_util.c" 
    "version_softap.c" 
    "proto/smak.pb.c" 
    "smak_conn.c" 
    "proto/pb_common.c" 
    "proto/pb_decode.c" 
    "proto/pb_encode.c"
    "smak_ota.c"
  INCLUDE_DIRS 
    "." 
    "include/" 
    "proto/" 
    "../nanopb/"
  PRIV_REQUIRES
    "cjson"
    "esp_http_client "
    "app_update "
    "esp_https_ota"
    "nvs_flash "
    "esp_netif "
    "esp_wifi "
    "protocomm"
    "mbedtls"
    "esp_driver_gpio"
)


