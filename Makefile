# Location of top-level MicroPython directory
MPY_DIR = ../../..

# Name of module
MOD = PeanutGB

#USER_C_MODULES += dma_pwm

# Source files (.c or .py)
SRC = peanut_sdl.c minigb_apu.c

# Architecture to build for (x86, x64, armv7m, xtensa, xtensawin)
ARCH = armv8m

# Include to get the rules for compiling and linking the module
include $(MPY_DIR)/py/dynruntime.mk
