############################################################################Definition############################################################################
# path
WEBCAM_DIR := $(shell pwd)
LIB_DIR := $(WEBCAM_DIR)/library_goke

# hardware
SENSOR0_TYPE := GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT
CHIPSET := gk7205v200

# library
PORTING_LIB_MODE := static
#PORTING_LIB_MODE := share
LIB_DIR_ITEM := aliyun freetype2 goke hisi libjpeg-turbo mp4v2 json-c zbar osdBase








############################################################################Toolchain############################################################################
#toolchain
TOOLCHAINS = /home/ning/toolchain/arm-gcc6.3-linux-uclibceabi/bin/arm-gcc6.3-linux-uclibceabi
CPP = $(TOOLCHAINS)-g++
CC  = $(TOOLCHAINS)-gcc
LD  = $(TOOLCHAINS)-ld
AR  = $(TOOLCHAINS)-ar
STRIP  = $(TOOLCHAINS)-strip
AT := 

#sdk flags
#SDK_USR_CFLAGS := -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -Wall -fPIC -O2 -fno-aggressive-loop-optimizations -ldl -ffunction-sections -fdata-sections -fstack-protector-strong -std=gnu99 -O0 -Wall -g2 -ggdb
SDK_USER_CFLAGS := -std=gnu99 -O0 -Wall -g2 -ggdb -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -Wall -fPIC
SDK_LD_CFLAGS := -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -fno-aggressive-loop-optimizations -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now,-s -ldl -fPIC -std=gnu99 -O0 -Wall -g2 -ggdb 
SDK_KER_CFLAGS := 

PORTING_CFLAGS += $(SDK_USR_CFLAGS)
PORTING_CFLAGS += -DUSER_BIT_32 -DKERNEL_BIT_32 -Wno-date-time -D_GNU_SOURCE
PORTING_CFLAGS += -DSENSOR0_TYPE=$(SENSOR0_TYPE) -DHI_ACODEC_TYPE_INNER -DHI_VQE_USE_STATIC_MODULE_REGISTER -DHI_AAC_USE_STATIC_MODULE_REGISTER -DHI_AAC_HAVE_SBR_LIB -DCHIPSET=$(CHIPSET)
PORTING_CFLAGS += -DLINK_VISUAL_ENABLE -DDEVICE_MODEL_GATEWAY -DDM_UNIFIED_SERVICE_POST -DDLL_IOT_EXPORTS -DAWSS_DISABLE_ENROLLEE  -DAWSS_DISABLE_REGISTRAR -DAWSS_SUPPORT_ADHA -DAWSS_SUPPORT_AHA
PORTING_CFLAGS += -DAWSS_SUPPORT_APLIST -DAWSS_SUPPORT_PHONEASAP -DAWSS_SUPPORT_ROUTER -DAWSS_SUPPORT_SMARTCONFIG -DAWSS_SUPPORT_SMARTCONFIG_WPS -DAWSS_SUPPORT_ZEROCONFIG -DCOAP_SERV_MULTITHREAD
PORTING_CFLAGS += -DCONFIG_HTTP_AUTH_TIMEOUT=5000 -DCONFIG_MID_HTTP_TIMEOUT=5000 -DCONFIG_GUIDER_AUTH_TIMEOUT=10000 -DDEVICE_MODEL_ENABLED -DDEV_BIND_ENABLED -DFORCE_SSL_VERIFY -DMQTT_COMM_ENABLED
PORTING_CFLAGS += -DWITH_MQTT_JSON_FLOW=1 -DWITH_MQTT_ZIP_TOPIC=1 -DWITH_MQTT_SUB_SHORTCUT=1 -DCOMPATIBLE_LK_KV -DMQTT_DIRECT -DOTA_ENABLED -DOTA_SIGNAL_CHANNEL=1 -DSUPPORT_TLS -DWIFI_PROVISION_ENABLED
PORIING_CFLAGS += -D_PLATFORM_IS_HOST_ -D_PLATFORM_IS_LINUX_ -DCONFIG_SDK_THREAD_COST -DENABLE_JPEGEDCF





############################################################################Target webcam############################################################################
TARGET := webcam

#webcam source
SRCS := $(wildcard $(WEBCAM_DIR)/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/common/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/common/buffer/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/common/cJSON/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/common/config/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/common/json/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/global/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/aliyun/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/audio/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/device/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/goke/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/goke/hisi/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/goke/hisi/adp/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/goke/hisi/bmp/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/manager/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/player/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/recorder/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/server/video/*.c)
#smart living source
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/components/timer_services/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/log/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/system/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/system/facility/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/utils/digest/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/utils/misc/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/infra/utils/kv/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/protocol/mqtt/client/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/protocol/mqtt/MQTTPacket/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/protocol/coap/local/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/ref-impl/hal/os/ubuntu/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/ref-impl/hal/ssl/mbedtls/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/ref-impl/tls/library/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/sdk-impl/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/awss/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/dev_bind/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/dev_bind/os/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/dev_diagnosis/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/linkkit/cm/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/linkkit/dm/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/linkkit/dm/client/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/linkkit/dm/server/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/linkkit/dev_reset/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/ota/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/smart_living/src/services/ota/impl/*.c)
#gmp source
SRCS += $(wildcard $(WEBCAM_DIR)/gmp/modules/isp/user/sensor/gk7205v200/galaxycore_gc2053/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/gmp/modules/isp/user/firmware/arch/gk7205v200/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/gmp/modules/isp/user/firmware/arch/gk7205v200/hal/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/gmp/modules/isp/user/firmware/src/main/*.c)
SRCS += $(wildcard $(WEBCAM_DIR)/gmp/modules/isp/user/firmware/arch/gk7205v200/algorithms/*.c)

#objs
OBJS := $(patsubst %.c, %.o, $(SRCS))















############################################################################Source include############################################################################
# third party
LIBRARY_INC := $(foreach item,$(LIB_DIR_ITEM),-I$(LIB_DIR)/$(item)/include)
LIBRARY_INC += -I$(LIB_DIR)/aliyun/include/exports -I$(LIB_DIR)/aliyun/include/imports -I$(LIB_DIR)/aliyun/linkvisual

# gmp
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/kernel/arch/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/kernel/arch/gk7205v200/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/kernel/mkp/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/kernel/mkp/include/gk7205v200/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/user/3a/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/user/firmware/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/user/firmware/arch/gk7205v200/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/isp/ext_inc
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/osal/include
LIBRARY_INC += -I$(WEBCAM_DIR)/gmp/modules/osal/linux/kernel

#lv sdk
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/include/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/include/imports
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/include/exports
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/log
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/system
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/system/facility
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/utils/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/utils/digest/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/utils/kv/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/infra/utils/misc/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/alcs/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/coap/cloud
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/coap/local
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/http
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/http2
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/http2/nghttp2
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/mqtt
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/protocol/mqtt/MQTTPacket
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/ref-impl/tls/include/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/ref-impl/hal/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/ref-impl/hal/os/ubuntu/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/sdk-impl
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/security/pro/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/security/pro/crypto/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/security/pro/id2/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/security/pro/itls/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/security/pro/km/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/awss/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/dev_bind/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/dev_bind/os
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/dev_diagnosis
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/https_stream
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit/dm
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit/dm/client
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit/dm/server
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit/dev_reset
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/linkkit/cm/include
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/mdal
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/mdal/mal
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/mdal/sal
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/mdal/sal/include
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/ota
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/shadow
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/services/awss
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/src/tools/
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/examples
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/examples/linkkit
LIBRARY_INC += -I$(WEBCAM_DIR)/smart_living/tests

############################################################################Link library############################################################################
PORTING_CFLAGS += $(LIBRARY_INC)

SYS_LIBS := -ljson-c -lpthread -lm -lc -lrt -ldl -lstdc++

# gmp base
GMP_LIBS := -lsecurec -laac_comm -ldehaze -ldnvqe -ldrc -lir_auto -lldci -lupvqe -lvoice_engine
GMP_LIBS += -lsns_ar0237 -lsns_f37 -lsns_gc2053 -lsns_gc2053_forcar -lsns_imx290 -lsns_imx307_2l -lsns_imx307
GMP_LIBS += -lsns_imx327_2l -lsns_imx327 -lsns_imx335 -lsns_os05a -lsns_ov2718 -lsns_sc2231 -lsns_sc2235 -lsns_sc3235 -lsns_sc4236
GMP_LIBS += -lsns_sc3335 -lsns_sc500ai -lsns_gc4653_2l

# gmp api
GMP_LIBS += -laac_dec -laac_enc -laac_sbr_dec -laac_sbr_enc -lgk_ae -lgk_api -lgk_awb -lgk_bcd
GMP_LIBS += -lgk_cipher -lgk_isp -lgk_ive -lgk_ivp -lgk_md -lgk_qr -lgk_tde
GMP_LIBS += -lvqe_aec -lvqe_agc -lvqe_anr -lvqe_eq -lvqe_hpf -lvqe_record -lvqe_res -lvqe_talkv2 -lvqe_wnr

# porting library
HI_LIBS += -lhi_aacdec -lhi_aacsbrdec -lhi_ae -lhi_bcd -lhi_isp -lhi_ivp -lhi_mpi -lhi_tde -lhi_vqe_agc -lhi_vqe_eq  -lhi_vqe_record -lhi_vqe_talkv2
HI_LIBS += -lhi_aacenc -lhi_aacsbrenc -lhi_awb -lhi_cipher -lhi_ive -lhi_md -lhi_qr -lhi_vqe_aec -lhi_vqe_anr -lhi_vqe_hpf -lhi_vqe_res -lhi_vqe_wnr
#HI_LIBS += -lhi_sample_common

# third party library
THIRD_PARTY_LIBS := -lfreetype -llink_visual_device -lmp4v2 -lzbar -ljpeg -losdBase

# library path
LIBRARY_PATH := $(foreach item,$(LIB_DIR_ITEM),-L$(LIB_DIR)/$(item)/lib)
LIBRARY_PATH += -L$(LIB_DIR)/aliyun/linkvisual

#
GK_STATIC_LIB_DIR     := $(LIB_DIR)/goke/lib/static
GK_SHARED_LIB_DIR     := $(LIB_DIR)/goke/lib/share


ifeq ($(PORTING_LIB_MODE), static)
PORTING_LIBS += -L$(GK_STATIC_LIB_DIR) $(LIBRARY_PATH)
PORTING_LIBS += -Wl,-Bstatic,--start-group $(GMP_LIBS) $(HI_LIBS) $(THIRD_PARTY_LIBS) -Wl,--end-group,-Bdynamic $(SYS_LIBS)
else ifeq ($(PORTING_LIB_MODE), share)
PORTING_LIBS += -L$(GK_SHARED_LIB_DIR) $(LIBRARY_PATH)
PORTING_LIBS += -shared -Wl,--start-group $(GMP_LIBS) $(HI_LIBS) $(THIRD_PARTY_LIBS) $(SYS_LIBS) -Wl,--end-group
endif

CFLAGS := $(PORTING_CFLAGS) $(PORTING_LIBS)
#CFLAGS += -I$(PORTING_ROOT_DIR)/sample/audio/adp





############################################################################Make rules############################################################################
.PHONY: all clean

all: $(OBJS)
	$(AT)$(CC) -o $(TARGET) $^ $(CFLAGS)
	@rm -f /home/ning/tftp-root/bin/webcam
	@cp webcam /home/ning/tftp-root/bin/
%.o : %.c
	$(AT)$(CC) -c -o $@ $< $(CFLAGS)

clean:
	$(AT)rm -rf $(OBJS) $(TARGET)





