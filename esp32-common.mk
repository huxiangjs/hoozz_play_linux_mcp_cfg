# Makefile
SRC_DIR := esp32-common
CFLAGS += -DUSE_TINY_AES_C
# SRCS_C += $(wildcard $(SRC_DIR)/*.c)
SRCS_C += $(SRC_DIR)/simple_ctrl.c \
          $(SRC_DIR)/crypto.c \
          $(SRC_DIR)/tiny-AES-c/aes.c \
	  $(SRC_DIR)/event_bus.c
