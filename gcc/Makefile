#******************************************************************************
#
# Makefile - Rules for building the freertos-demo application.
#
#
#  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
#
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#*****************************************************************************

PROJ_NAME=bigblackprogrammer

#
# The base directory.
#
SDK_PATH=/home/kript0n/Applications/EmbeddedArm/ti/cc3200-sdk

ROOT=$(SDK_PATH)
PROJ_PATH=..

#
# Include the common make definitions.
#
include ${ROOT}/tools/gcc_scripts/makedefs

#
# Where to find source files that do not live in this directory.
#
VPATH=$(PROJ_PATH)
VPATH+=$(SDK_PATH)/drivers
VPATH+=$(SDK_PATH)/common
VPATH+=$(SDK_PATH)/driverlib
VPATH+=$(SDK_PATH)/middleware/driver
VPATH+=$(SDK_PATH)/middleware/driver/hal
VPATH+=$(SDK_PATH)/middleware/framework/pm


#
# Additional Compiler Flags
#
CFLAGS+=-DUSE_FREERTOS -DSL_PLATFORM_MULTI_THREADED

#
# Generate map file
#
LDFLAGS +=-Map=$(OBJDIR)/$(PROJ_NAME).map

#
# Where to find header files that do not live in the source directory.
#
IPATH=$(PROJ_PATH)
IPATH+=$(SDK_PATH)
IPATH+=$(SDK_PATH)/common
IPATH+=$(SDK_PATH)/inc
IPATH+=$(SDK_PATH)/oslib
IPATH+=$(SDK_PATH)/driverlib

IPATH+=$(SDK_PATH)/third_party/FreeRTOS
IPATH+=$(SDK_PATH)/third_party/FreeRTOS/source
IPATH+=$(SDK_PATH)/third_party/FreeRTOS/source/portable/GCC/ARM_CM4
IPATH+=$(SDK_PATH)/third_party/FreeRTOS/source/include

IPATH+=$(SDK_PATH)/middleware/driver
IPATH+=$(SDK_PATH)/middleware/driver/hal
IPATH+=$(SDK_PATH)/middleware/framework/pm

IPATH+=$(SDK_PATH)/simplelink
IPATH+=$(SDK_PATH)/simplelink/source
IPATH+=$(SDK_PATH)/simplelink/include
IPATH+=$(SDK_PATH)/simplelink_extlib/provisioninglib

#
# The default rule, which causes the driver library to be built.
#
all: ${OBJDIR} ${BINDIR}
all: ${BINDIR}/$(PROJ_NAME).axf

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${OBJDIR} ${wildcard *~}
	@rm -rf ${BINDIR} ${wildcard *~}


#
# The rule to create the target directories.
#
${OBJDIR}:
	@mkdir -p ${OBJDIR}

${BINDIR}:
	@mkdir -p ${BINDIR}

#
# Rules for building the freertos_demo example.
#
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/main.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/pinmux.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/wlan_config.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/logging.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/udp_resolver.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/wired_configurator.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/sys.o

${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/controller.o

${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/programmer.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/programmer_parser.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/bridge.o

${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/packets.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/packet_handler.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/packet_manager.o

${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/pool.o


# Common drivers
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/pin.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/gpio.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/gpio_if.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/uart_if.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/udma_if.o

# Network bindings
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/network_common.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/network_if.o

# Middleware Drivers
# ${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/uart_hal.o
# ${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/uart_drv.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/spi_hal.o
${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/spi_drv.o
# ${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/cc_pm.o

# Simple link library
${BINDIR}/$(PROJ_NAME).axf: ${ROOT}/simplelink/${COMPILER}/${BINDIR}/libsimplelink.a

${BINDIR}/$(PROJ_NAME).axf: ${OBJDIR}/startup_${COMPILER}.o
${BINDIR}/$(PROJ_NAME).axf: ${ROOT}/driverlib/${COMPILER}/${BINDIR}/libdriver.a

# Free-RTOS library
${BINDIR}/$(PROJ_NAME).axf: ${ROOT}/oslib/${COMPILER}/${BINDIR}/FreeRTOS.a

SCATTERgcc_$(PROJ_NAME)=$(PROJ_NAME).ld
ENTRY_$(PROJ_NAME)=ResetISR


#
# Include the automatically generated dependency files.
#
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
