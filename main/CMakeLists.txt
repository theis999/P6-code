
idf_component_register(
  SRCS 
    "smak_util.c" 
    "usb.c" 
    "version_softap.c" 
    "proto/smak.pb.c" 
    "smak_conn.c" 
    "proto/pb_common.c" 
    "proto/pb_decode.c" 
    "proto/pb_encode.c"
  INCLUDE_DIRS 
    "." 
    "include/" 
    "proto/" 
    "../nanopb/"
)

