cmake_minimum_required(VERSION 3.19)
project(webcam)

set(SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(LIBRARY_ROOT "${SDK_ROOT}/library_goke")
set(LV_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/smart_living")
set(GMP_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/gmp")

SET(CMAKE_C_COMPILER ${SDK_ROOT}/tools/toolchains/arm-gcc6.3-linux-uclibceabi/bin/arm-gcc6.3-linux-uclibceabi-gcc)
message("${LIBRARY_ROOT}")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "-std=gnu99 -O0 -Wall -g2 -ggdb -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -Wall -fPIC")
set(CMAKE_C_FLAGS "-g")
set(CMAKE_C_FLAGS "-Wall ")
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(
        "${LIBRARY_ROOT}/goke/include"
        "${LIBRARY_ROOT}/json-c/include"
        "${LIBRARY_ROOT}/mp4v2/include"
        "${LIBRARY_ROOT}/aliyun/linkvisual"
        "${LIBRARY_ROOT}/zbar/include"
        "${LIBRARY_ROOT}/freetype2/include"
        "${LIBRARY_ROOT}/openssl/include"
        "${LIBRARY_ROOT}/libjpeg-turbo/include"
        "${LIBRARY_ROOT}/osdBase/include"

        ###gmp library###
        "${GMP_SDK_ROOT}/include/"
        "${GMP_SDK_ROOT}/modules/isp/include"
        "${GMP_SDK_ROOT}/modules/isp/kernel/arch/include"
        "${GMP_SDK_ROOT}/modules/isp/kernel/arch/gk7205v200/include/"
        "${GMP_SDK_ROOT}/modules/isp/kernel/mkp/include/"
        "${GMP_SDK_ROOT}/modules/isp/kernel/mkp/include/gk7205v200/include"
        "${GMP_SDK_ROOT}/modules/isp/user/3a/include"
        "${GMP_SDK_ROOT}/modules/isp/user/firmware/include"
        "${GMP_SDK_ROOT}/modules/isp/user/firmware/arch/gk7205v200/include"
        "${GMP_SDK_ROOT}/modules/isp/ext_inc"
        "${GMP_SDK_ROOT}/modules/osal/include"
        "${GMP_SDK_ROOT}/modules/osal/linux/kernel"

        ###lv sdk###
        ${LV_SDK_ROOT}/include/
        ${LV_SDK_ROOT}/include/imports
        ${LV_SDK_ROOT}/include/exports
        ${LV_SDK_ROOT}/src/infra/log
        ${LV_SDK_ROOT}/src/infra/system
        ${LV_SDK_ROOT}/src/infra/system/facility
        ${LV_SDK_ROOT}/src/infra/utils/
        ${LV_SDK_ROOT}/src/infra/utils/digest/
        ${LV_SDK_ROOT}/src/infra/utils/kv/
        ${LV_SDK_ROOT}/src/infra/utils/misc/
        ${LV_SDK_ROOT}/src/protocol/alcs/
        ${LV_SDK_ROOT}/src/protocol/coap/cloud
        ${LV_SDK_ROOT}/src/protocol/coap/local
        ${LV_SDK_ROOT}/src/protocol/http
        ${LV_SDK_ROOT}/src/protocol/http2
        ${LV_SDK_ROOT}/src/protocol/http2/nghttp2
        ${LV_SDK_ROOT}/src/protocol/mqtt
        ${LV_SDK_ROOT}/src/protocol/mqtt/MQTTPacket
        ${LV_SDK_ROOT}/src/ref-impl/tls/include/
        ${LV_SDK_ROOT}/src/ref-impl/hal/
        ${LV_SDK_ROOT}/src/ref-impl/hal/os/ubuntu/
        ${LV_SDK_ROOT}/src/sdk-impl
        ${LV_SDK_ROOT}/src/security/pro/
        ${LV_SDK_ROOT}/src/security/pro/crypto/
        ${LV_SDK_ROOT}/src/security/pro/id2/
        ${LV_SDK_ROOT}/src/security/pro/itls/
        ${LV_SDK_ROOT}/src/security/pro/km/
        ${LV_SDK_ROOT}/src/services/awss/
        ${LV_SDK_ROOT}/src/services/dev_bind/
        ${LV_SDK_ROOT}/src/services/dev_bind/os
        ${LV_SDK_ROOT}/src/services/dev_diagnosis
        ${LV_SDK_ROOT}/src/services/https_stream
        ${LV_SDK_ROOT}/src/services/linkkit
        ${LV_SDK_ROOT}/src/services/linkkit/dm
        ${LV_SDK_ROOT}/src/services/linkkit/dm/client
        ${LV_SDK_ROOT}/src/services/linkkit/dm/server
        ${LV_SDK_ROOT}/src/services/linkkit/dev_reset
        ${LV_SDK_ROOT}/src/services/linkkit/cm/include
        ${LV_SDK_ROOT}/src/services/mdal
        ${LV_SDK_ROOT}/src/services/mdal/mal
        ${LV_SDK_ROOT}/src/services/mdal/sal
        ${LV_SDK_ROOT}/src/services/mdal/sal/include
        ${LV_SDK_ROOT}/src/services/ota
        ${LV_SDK_ROOT}/src/services/shadow
        ${LV_SDK_ROOT}/src/services/awss
        ${LV_SDK_ROOT}/src/tools/
        ${LV_SDK_ROOT}/examples
        ${LV_SDK_ROOT}/examples/linkkit
        ${LV_SDK_ROOT}/tests
)

file(GLOB SDK_SOURCES
        #sdk
        ###timer_service###
        "smart_living/components/timer_service/*.c"
        ###infra_log###
        "smart_living/src/infra/log/*.c"
        ###infra_system###
        "smart_living/src/infra/system/*.c"
        "smart_living/src/infra/system/facility/*.c"
        ###infra_utility###
        "smart_living/src/infra/utils/digest/*.c"
        "smart_living/src/infra/utils/misc/*.c"
        "smart_living/src/infra/utils/kv/*.c"
        ###protocol_mqtt###
        "smart_living/src/protocol/mqtt/client/*.c"
        "smart_living/src/protocol/mqtt/MQTTPacket/*.c"
        ###protocol_local_coap###
        "smart_living/src/protocol/coap/local/*.c"
        ###ref_impl_hal###
        "smart_living/src/ref-impl/hal/os/ubuntu/*.c"
        "smart_living/src/ref-impl/hal/ssl/mbedtls/*.c"
        ###ref_impl_tls###
        "smart_living/src/ref-impl/tls/library/*.c"
        ###sdk###
        "smart_living/src/sdk-impl/*.c"
        ###awss###
        "smart_living/src/services/awss/*.c"
        ###dev_bind###
        "smart_living/src/services/dev_bind/*.c"
        "smart_living/src/services/dev_bind/os/*.c"
        ###dev_diag###
        "smart_living/src/services/dev_diagnosis/*.c"
        ###linkit_cm###
        "smart_living/src/services/linkkit/cm/*.c"
        ###linkit_dm###
        "smart_living/src/services/linkkit/dm/*.c"
        "smart_living/src/services/linkkit/dm/client/*.c"
        "smart_living/src/services/linkkit/dm/server/*.c"
        ###linkit_devreset###
        "smart_living/src/services/linkkit/dev_reset/*.c"
        ###ota###
        "smart_living/src/services/ota/*.c"
        "smart_living/src/services/ota/impl/*.c"
        ###awss###
        "smart_living/src/services/awss/*.c"
        )
file(GLOB SOURCES
        # webcam
        "main.c"
        "global/*.c"
        "server/manager/*.c"
        "server/device/*.c"
        "server/goke/*.c"
        "server/goke/hisi/*.c"
        "server/goke/hisi/adp/*.c"
        "server/goke/hisi/bmp/*.c"
        "server/audio/*.c"
        "server/video/*.c"
        "server/aliyun/*.c"
        "server/player/*.c"
        "server/recorder/*.c"
        "common/buffer/*.c"
        "common/cJSON/*.c"
        "common/config/*.c"
        "common/json/*.c"
        "common/memory/*.c"
        "common/*.c"
        )

file(GLOB GMP_SOURCES
        "gmp/modules/isp/user/sensor/gk7205v200/galaxycore_gc2053/*.c"
        "gmp/modules/isp/user/firmware/arch/gk7205v200/*.c"
        "gmp/modules/isp/user/firmware/arch/gk7205v200/hal/*.c"
        "gmp/modules/isp/user/firmware/src/main/*.c"
        "gmp/modules/isp/user/firmware/arch/gk7205v200/algorithms/*.c"
#       "gmp/modules/isp/kernel/arch/gk7205v200/hal/*.c"
#       "gmp/modules/isp/kernel/init/linux/*.c"
#       "gmp/modules/isp/kernel/mkp/src/*.c"
        )

ADD_DEFINITIONS (-DLINK_VISUAL_ENABLE)
ADD_DEFINITIONS (-DDEVICE_MODEL_GATEWAY)
ADD_DEFINITIONS (-DDM_UNIFIED_SERVICE_POST)
ADD_DEFINITIONS (-DDLL_IOT_EXPORTS)
ADD_DEFINITIONS (-DAWSS_DISABLE_ENROLLEE)
ADD_DEFINITIONS (-DAWSS_DISABLE_REGISTRAR)
ADD_DEFINITIONS (-DAWSS_SUPPORT_ADHA)
ADD_DEFINITIONS (-DAWSS_SUPPORT_AHA)
ADD_DEFINITIONS (-DAWSS_SUPPORT_APLIST)
ADD_DEFINITIONS (-DAWSS_SUPPORT_PHONEASAP)
ADD_DEFINITIONS (-DAWSS_SUPPORT_ROUTER)
ADD_DEFINITIONS (-DAWSS_SUPPORT_SMARTCONFIG)
ADD_DEFINITIONS (-DAWSS_SUPPORT_SMARTCONFIG_WPS)
ADD_DEFINITIONS (-DAWSS_SUPPORT_ZEROCONFIG)
ADD_DEFINITIONS (-DCOAP_SERV_MULTITHREAD)
ADD_DEFINITIONS (-DCONFIG_HTTP_AUTH_TIMEOUT=5000)
ADD_DEFINITIONS (-DCONFIG_MID_HTTP_TIMEOUT=5000)
ADD_DEFINITIONS (-DCONFIG_GUIDER_AUTH_TIMEOUT=10000)
ADD_DEFINITIONS (-DDEVICE_MODEL_ENABLED)
ADD_DEFINITIONS (-DDEV_BIND_ENABLED)
ADD_DEFINITIONS (-DFORCE_SSL_VERIFY)
ADD_DEFINITIONS (-DMQTT_COMM_ENABLED)
ADD_DEFINITIONS (-DWITH_MQTT_JSON_FLOW=1)
ADD_DEFINITIONS (-DWITH_MQTT_ZIP_TOPIC=1)
ADD_DEFINITIONS (-DWITH_MQTT_SUB_SHORTCUT=1)
ADD_DEFINITIONS (-DCOMPATIBLE_LK_KV)
ADD_DEFINITIONS (-DMQTT_DIRECT)
ADD_DEFINITIONS (-DOTA_ENABLED)
ADD_DEFINITIONS (-DOTA_SIGNAL_CHANNEL=1)
ADD_DEFINITIONS (-DSUPPORT_TLS)
ADD_DEFINITIONS (-DWIFI_PROVISION_ENABLED)
ADD_DEFINITIONS (-D_PLATFORM_IS_HOST_)
ADD_DEFINITIONS (-D_PLATFORM_IS_LINUX_)
ADD_DEFINITIONS (-DCONFIG_SDK_THREAD_COST)
ADD_DEFINITIONS (-DENABLE_JPEGEDCF)

add_executable(webcam ${GMP_SOURCES} ${SDK_SOURCES} ${SOURCES} )
target_compile_definitions(webcam PUBLIC HI_ACODEC_TYPE_INNER)
target_compile_definitions(webcam PUBLIC HI_AAC_HAVE_SBR_LIB)
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_mpi.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_isp.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_ae.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_awb.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_ive.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_ivp.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libhi_md.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/mp4v2/lib/libmp4v2.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/zbar/lib/libzbar.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/freetype2/lib/libfreetype.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_api.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_isp.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_awb.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_ae.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_ive.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_ivp.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libgk_md.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libdnvqe.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libupvqe.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvqe_hpf.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvqe_anr.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvqe_aec.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvqe_agc.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvqe_eq.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libsecurec.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libsns_gc2053.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libdrc.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libir_auto.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libdehaze.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libldci.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/libvoice_engine.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/goke/lib/base/libisp.a")

target_link_libraries(webcam "${LIBRARY_ROOT}/aliyun/linkvisual/liblink_visual_device.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/aliyun/lib/libiot_sdk.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/aliyun/lib/libiot_hal.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/aliyun/lib/libiot_tls.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/mbedtls/lib/libmbedtls.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/mbedtls/lib/libmbedx509.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/mbedtls/lib/libmbedcrypto.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/json-c/lib/libjson-c.so")
#target_link_libraries(webcam "${LIBRARY_ROOT}/libpng/lib/libpng.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/libjpeg-turbo/lib/libjpeg.a")
target_link_libraries(webcam "${LIBRARY_ROOT}/osdBase/lib/libosdBase.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/zlib/lib/libz.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/openssl/lib/libssl.a")
#target_link_libraries(webcam "${LIBRARY_ROOT}/openssl/lib/libcrypto.a")
target_link_libraries(webcam -lpthread -lm -lc -lrt -ldl -lstdc++)

add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND cp -f webcam /home/ning/tftp-root/bin
        )