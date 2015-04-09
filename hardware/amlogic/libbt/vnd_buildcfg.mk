intermediates := $(local-intermediates-dir)

ifeq ($(BCM_BLUETOOTH_LPM_ENABLE),true)
SRC := $(call my-dir)/include/$(addprefix vnd_, $(addsuffix _lpm.txt,$(basename $(TARGET_BOARD_PLATFORM))))
#SRC := $(call my-dir)/include/vnd_40183_lpm.txt
else
SRC := $(call my-dir)/include/$(addprefix vnd_, $(addsuffix .txt,$(basename $(TARGET_BOARD_PLATFORM))))
#SRC := $(call my-dir)/include/vnd_40183.txt
endif
ifeq (,$(wildcard $(SRC)))
# configuration file does not exist. Use default one
SRC := $(call my-dir)/include/vnd_generic.txt
endif
GEN := $(intermediates)/vnd_buildcfg.h
TOOL := $(TOP_DIR)external/bluetooth/bluedroid/tools/gen-buildcfg.sh

$(GEN): PRIVATE_PATH := $(call my-dir)
$(GEN): PRIVATE_CUSTOM_TOOL = $(TOOL) $< $@
$(GEN): $(SRC)  $(TOOL)
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)
