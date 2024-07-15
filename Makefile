include makedefs

RTOS_SOURCE_DIR=/home/gaston/Descargas/FreeRTOSv202212.01/FreeRTOS/Source

CFLAGS+=-I . -I ${RTOS_SOURCE_DIR}/include -I ${RTOS_SOURCE_DIR}/portable/GCC/ARM_CM3 -D GCC_ARMCM3_LM3S811 -D inline= -g -O0

VPATH=${RTOS_SOURCE_DIR}:${RTOS_SOURCE_DIR}/portable/MemMang:${RTOS_SOURCE_DIR}/portable/GCC/ARM_CM3

OBJS=${COMPILER}/main.o \
     ${COMPILER}/list.o \
     ${COMPILER}/queue.o \
     ${COMPILER}/tasks.o \
     ${COMPILER}/port.o \
     ${COMPILER}/syscalls.o \
     ${COMPILER}/heap_1.o \
     ${COMPILER}/BlockQ.o \
     ${COMPILER}/PollQ.o \
     ${COMPILER}/integer.o \
     ${COMPILER}/semtest.o \
     ${COMPILER}/osram96x16.o 

INIT_OBJS= ${COMPILER}/startup.o

LIBS= hw_include/libdriver.a

#
# The default rule, which causes init to be built.
#
all: ${COMPILER} ${COMPILER}/RTOSDemo.axf

#
# The rule to clean out all the build products
#
clean:
	@rm -rf ${COMPILER} ${wildcard *.bin} RTOSDemo.axf

#
# The rule to create the target directory
#
${COMPILER}:
	@mkdir ${COMPILER}

# ${COMPILER}/RTOSDemo.axf: ${OBJS}
# 	@${LD} -T standalone.ld --entry ResetISR ${LDFLAGS} -o ${@} ${^} '${LIBC}' '${LIBGCC}'
# 	@${OBJCOPY} -O binary ${@} ${@:.axf=.bin}
${COMPILER}/RTOSDemo.axf: ${INIT_OBJS} ${OBJS} ${LIBS}
SCATTER_RTOSDemo=standalone.ld
ENTRY_RTOSDemo=ResetISR

#
# Include the automatically generated dependency files.
#
-include ${wildcard ${COMPILER}/*.d} __dummy__
