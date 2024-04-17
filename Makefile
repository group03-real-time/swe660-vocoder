-include config.mk

# --- Makefile Setup ---
# The following components of the makefile are intended to be the main ones
# we need to touch while working on the ode.

# Name of the compiled application
TARGET=vocoder

# List of source files
SRCS=\
	gpio.c\
	gpio_mmap.c\
	app.c\
	gpio_pins.c\
	good_user_input.c\
	main.c\
	menu.c\
	hardware.c\
	audio_params.c\
	subapps/offline_vocode.c\
	subapps/offline_synth.c\
	subapps/offline_vocode_synth.c\
	subapps/pru_play_wav.c\
	subapps/pru_record_wav.c\
	subapps/2x2_button_grid.c\
	pru/pru_interface.c\
	dsp/bpf.c\
	dsp/vocoder.c\
	dsp/synth.c\
	wav/wav.c\
	
# List of subdirectories inside src. Needed to keep the build fast.
SRC_DIRECTORIES=dsp wav pru subapps

# List of flags we want for the C compiler
CFLAGS_DEFAULT=-Wall -std=gnu11 -O3 -Wno-error=unused-result -g -I. -Isrc
LDFLAGS_DEFAULT=-lm

# The default SSH target, or whatever, for the beaglebone. Can be overridden
# if needed. Used for automatically running on the beaglebone.
BEAGLEBONE_SSH?=debian@192.168.7.2

# --- Makefile Implementation ---
# What follows is the actual "implementation" of the makefile.

# Phony targets: do not correspond to real files. Used to provide little commands.
.PHONY: help all clean ssh conf-qemu conf-bbb conf-bbb-cross conf-qemu-and-cross firmware clean-firmware

# If we don't have a config.mk: Build three configurations:
# 1) for qemu
# 2) for cross, without mmap
# 3) for cross, with mmap
ifeq (,$(wildcard ./config.mk))
BUILDS=cross dsp32 dspfloat

DEFS_dspfloat=DSP_FLOAT
DEFS_dsp32=DSP_FIXED_32

CC_cross=arm-linux-gnueabihf-gcc
DEFS_cross=USE_MMAP_GPIO HARDWARE

IS_CROSS_cross=true
else
BUILDS?=default
endif
TARGETS=$(BUILDS:%=$(TARGET)-%)

PRU_SOURCES=adc.pru0.c i2sv1.pru1.c
PRU_HEADERS=

PRU_BUILD_DIR=build-firmware

PRU_TARGETS=$(PRU_SOURCES:%.c=vocoder-%.firmware)

PRU_CGT?=/usr
PRU_SUPPORT?=/usr/lib/ti/pru-software-support-package-v6.0

PRU_CC=$(PRU_CGT)/bin/clpru
PRU_LD=$(PRU_CGT)/bin/lnkpru

PRU_CFLAGS=--include_path=$(PRU_SUPPORT)/include \
   --include_path=$(PRU_SUPPORT)/include/am335x \
   --include_path=$(PRU_CGT)/include \
   -v3 -O2 --printf_support=minimal --display_error_number --endian=little --hardware_mac=on \
   --obj_directory=$(PRU_BUILD_DIR) --pp_directory=$(PRU_BUILD_DIR) --asm_directory=$(PRU_BUILD_DIR) -ppd -ppa --asm_listing \
   --c_src_interlist

PRU_LDFLAGS=--reread_libs --warn_sections --stack_size=0x100 --heap_size=0x100  \
   -i$(PRU_CGT)/lib -i$(PRU_CGT)/include firmware/am335x_pru.cmd --library=libc.a \
   --library=$(PRU_SUPPORT)/lib/rpmsg_lib.lib

# The top-level build rule / PHONY target all -- just corresponds to building
# the final executable. Used to make it clear where the rules begin.
all: $(TARGETS) firmware

firmware: $(PRU_TARGETS)

clean-firmware:
	rm -rf $(PRU_BUILD_DIR)
	rm -f $(PRU_TARGETS)

# Build rule for PRU firmware
vocoder-%.firmware: firmware/%.c | $(PRU_BUILD_DIR)
	$(PRU_CC) -fe $(PRU_BUILD_DIR)/%.o $< $(PRU_CFLAGS)
	$(PRU_LD) -o $@ $(PRU_BUILD_DIR)/%.o $(PRU_LDFLAGS)

$(PRU_BUILD_DIR):
	mkdir -p $@

define RUN_CROSS_template = 

.PHONY: run-$(1) scp-$(1)

# Defines a target to run the application on the beaglebone. Doesn't support
# providing input for the app, although I suppose it could be tweaked so that
# we remain ssh'd in.
run-$(1): $$(TARGET_$(1))
	scp $$(TARGET_$(1)) $(BEAGLEBONE_SSH):~
ifdef SSH_INPUT_FILE
	scp $(SSH_INPUT_FILE) $(BEAGLEBONE_SSH):~
	echo "pidof curprogram | xargs kill; rm -f ~/curprogram; cp ~/$$(TARGET_$(1)) ~/curprogram; ~/curprogram &> /dev/null < $(SSH_INPUT_FILE) & exit" | ssh $(BEAGLEBONE_SSH)
else
	echo "pidof curprogram | xargs kill; rm -f ~/curprogram; cp ~/$$(TARGET_$(1)) ~/curprogram; ~/curprogram &> /dev/null < /dev/null & exit" | ssh $(BEAGLEBONE_SSH)
endif

scp-$(1): $$(TARGET_$(1)) $(PRU_TARGETS)
	scp $$^ $(BEAGLEBONE_SSH):~

endef

define BUILD_template =

# Set defaults for CC and DEFS when they aren't defined by the config.
# If we're cross compiling, we need to use CC=arm-linux-gnueabiehf-gcc, and
# if we're running on QEMU, we define EMULATOR so we know that we're on emulator.
CC_$(1)?=gcc
DEFS_$(1)?=

# The CFLAGS include:
# - Any CFLAGS defined on the command line
# - The standard CFLAGS we configured in CFLAGS_DEFAULT
# - A -D<Macro> flag for every macro defined in the DEFS variable.
CFLAGS_$(1):=$$(CFLAGS_$(1)) $$(CFLAGS) $$(CFLAGS_DEFAULT) $$(DEFS_$(1):%=-D%) -DTARGET_NAME='"$(TARGET)"'
CPPFLAGS_$(1):=$$(CPPFLAGS_$(1)) -O3 -g $$(DEFS_$(1):%=-D%) -DTARGET_NAME='"$(TARGET)"'
LDFLAGS_$(1):=$$(LDFLAGS) $$(LDFLAGS_DEFAULT)

# The build directories are the directories we need to exist for our compiled
# .o files. If they don't exist, gcc will complain it can't output to a
# non existing directory.
BUILD_ROOT_$(1)=build-$(1)
BUILD_DIRECTORIES_$(1):=$$(BUILD_ROOT_$(1)) $$(SRC_DIRECTORIES:%=$$(BUILD_ROOT_$(1))/%)

# The object files correspond to the source files as such:
#                    src/main.c <-> build/main.o
OBJS_$(1)=$$(SRCS:%.c=$$(BUILD_ROOT_$(1))/%.c.o)

#allbuilds:
#	CC=arm-linux-gnueabihf-gcc DEFS=EMULATOR BUILD_ROOT=build-qemu TARGET=$(TARGET)-qemu make all
#	BUILD_ROOT=build-cross TARGET=$(TARGET)-cross make all

TARGET_$(1)=$(TARGET)-$(1)

# The final compiled target executable, depends on the object files
# Rule: compile, using our CC compiler, all of the object files together.
#       Uses LDFLAGS as flags for the compiler (because we're actually
#       just using the compiler to invoke the linker.)
$$(TARGET_$(1)): $$(OBJS_$(1))
	$$(CC_$(1)) $$^ -o $$@ $$(LDFLAGS_$(1))

# Create a phony rule for building a specific version of the build, in case you
# have two builds. Example: with conf-qemu-and-cross, you can run make qemu
# to only rebuild qemu.
#
# Also create a rule for just cleaning this target.
#
# Finally, create a phony rule for outputting a single command that can do
# this compilation to stdout.
.PHONY: $(1) clean-$(1) cc-cmd-$(1)
$(1): $$(TARGET_$(1))

clean-$(1):
	rm -f $$(TARGET_$(1))
	rm -rf $$(BUILD_ROOT_$(1))

# Build rule for each .o file corresponding to a source file. We compile
# it (-c) to the corresponding .o. We use -MMD to ensure that dependency files
# are generated, so that make will know to rebuild when headers change.
$$(BUILD_ROOT_$(1))/%.c.o: src/%.c | $$(BUILD_DIRECTORIES_$(1))
	$$(CC_$(1)) -MMD -c $$< -o $$@ $$(CFLAGS_$(1))

cc-cmd-$(1):
	@echo $$(CC_$(1)) "src/*.c" -o $$(TARGET_$(1)) $$(CFLAGS_$(1)) $$(LDFLAGS_$(1))

# To actually create the build directories, we use mkdir. -p to ensure it 
# doesn't complain if they already exist.
$$(BUILD_DIRECTORIES_$(1)):
	mkdir -p $$@

$$(if $$(IS_CROSS_$(1)),$$(eval $$(call RUN_CROSS_template,$(1))),)

# We include all the generated dependency files so that 
-include $$(OBJS_$(1):%.o=%.d)

endef

# https://www.gnu.org/software/make/manual/html_node/Eval-Function.html
# Basic idea: for each of the names in BUILDS, we generate a relatively
# standard set of makefile rules using the eval command.
$(foreach build,$(BUILDS),$(eval $(call BUILD_template,$(build))))

# make clean cleans all the built files.
# Just rm -rf them all. this is necessary so rm doesn't complain if we've already
# cleaned something.
clean: clean-firmware
	rm -rf build-*
	rm -f $(TARGETS)

# make ssh: ssh into the beaglebone. easier than typing ssh debian@192.168.7.2
ssh:
	ssh $(BEAGLEBONE_SSH)

# make ssh-kill: ssh into the beaglebone and try to kill "curprogram"
ssh-kill:
	echo "pidof curprogram | xargs kill" | ssh $(BEAGLEBONE_SSH)

# ---Configuration generation commands ---
# these all generate a config.mk corresponding
# to a specific setup.

# For qemu, we need to use the cross compiler. We also need to use -static
# so that we can run the code in the qemu-arm.
#
# Also, define EMULATOR so our code knows we're using the emulator.
conf-qemu: clean
	@if test -f config.mk; then mv config.mk config.mk.old; fi
	@echo BUILDS=qemu > config.mk
	@echo CC_qemu=arm-linux-gnueabihf-gcc >> config.mk
	@echo 'LDFLAGS_qemu:=$$(LDFLAGS) -static' >> config.mk
	@echo DEFS_qemu=EMULATOR >> config.mk
	@echo Configuration written to config.mk.

# For the beaglebone, we can just use gcc (the default CC above). And, we
# don't need any link flags or DEFS. We could add CFLAGS:=$(CFLAGS) -O2, though,
# if we want the code to run faster on the beagle.
conf-bbb: clean
	@if test -f config.mk; then mv config.mk config.mk.old; fi
	@echo '# Empty config' > config.mk
	@echo Configuration written to config.mk.

# Generates a setup for cross compiling for the beagle. This means we use the
# cross compiler, but we don't define DEFS=EMULATOR, because we're running
# on the beagle, not on the emulator.
conf-bbb-cross: clean
	@if test -f config.mk; then mv config.mk config.mk.old; fi
	@echo BUILDS=cross > config.mk
	@echo CC_cross=arm-linux-gnueabihf-gcc >> config.mk
	@echo Configuration written to config.mk.

conf-qemu-and-cross: clean
	@if test -f config.mk; then mv config.mk config.mk.old; fi
	@echo BUILDS=qemu cross > config.mk
	@echo CC_qemu=arm-linux-gnueabihf-gcc >> config.mk
	@echo 'LDFLAGS_qemu:=$$(LDFLAGS) -static' >> config.mk
	@echo DEFS_qemu=EMULATOR >> config.mk
	@echo CC_cross=arm-linux-gnueabihf-gcc >> config.mk
	@echo Configuration written to config.mk.

ifdef TARGET_qemu

.PHONY: run-qemu

# Defines a PHONY target for running qemu. If you don't want to type qemu-arm.
run-qemu: $(TARGET_qemu)
	qemu-arm $<

endif

# Shows a help menu.
help:
	@echo available phony targets:
	@printf "\tmake all: build the target for all current configurations\n"
	@$(foreach build,$(BUILDS),printf "\tmake $(build): build only $(TARGET)-$(build)\n";)
	@echo
	@printf "\tmake clean: clean built files\n"
	@$(foreach build,$(BUILDS),printf "\tmake clean-$(build): clean built files for just $(build)\n";)
	@echo
	@printf "\tmake conf-qemu: generate config.mk for running in qemu\n"
	@printf "\tmake conf-bbb: generate config.mk for building and running on the BeagleBone Black\n"
	@printf "\tmake conf-bbb-cross: generate config.mk that builds with cross compiler, but targets the BeagleBone Black, not the emualtor\n"
	@printf "\tmake conf-qemu-and-cross: generate config.mk that builds both qemu and cross compiled for BBB\n"
	@echo
	@printf "\tmake help: show this menu\n"
	@echo
ifdef TARGET_cross
	@printf "\tmake run-cross: copies the compiled executable onto your BeagleBone over SSH, then runs it\n"
	@echo
endif
ifdef TARGET_qemu
	@printf "\tmake run-qemu: runs qemu-arm with the compiled executable\n"
	@echo
endif
	@printf "basic usage: 'make conf-qemu' to create config.mk, then 'make' to compile and 'qemu-arm $(TARGET)-qemu' to run\n"