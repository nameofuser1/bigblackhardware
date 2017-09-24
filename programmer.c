#include "programmer.h"

#include "hw_memmap.h"
#include "rom_map.h"
#include "prcm.h"
#include "spi.h"
#include "config.h"

#include "logging.h"


/* Current SPI bitrate */
static int spi_bitrate = PROG_SPI_DEFAULT_FREQ;

/* Used to resume thread */
static OsiSyncObj_t resume_obj;

/* Task prototype */
static void prog_task(void *pvParameters);


static _i16 enable_pgm_mode(void) {
}


static inline void spi_enable(void) {
    MAP_SPIReset(PROG_SPI_BASE);

    MAP_SPIConfigSetExpClk(PROG_SPI_BASE,
                           MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           spi_bitrate,
                           SPI_MODE_MASTER,
                           SPI_SUB_MODE_0,
                           (SPI_SW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_OFF |
                            SPI_CS_ACTIVELOW |
                            SPI_WL_8));

    MAP_SPIEnable(PROG_SPI_BASE);
}


static inline void spi_disable(void) {
    MAP_SPIDisable(PROG_SPI_BASE);
}


_i16 programmer_start(void) {
    _i16 status;

    status = osi_SyncObjCreate(&resume_obj);
    OSI_ASSERT_ON_ERROR(status);

    status = osi_TaskCreate(prog_task, PROGRAMMER_TASK_NAME, PROGRAMMER_TASK_STACK_SIZE,
                            NULL, PROGRAMMER_TASK_PRIO, NULL);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 programmer_resume(OsiMsgQ_t *in_queue, OsiMsgQ_t *out_queue) {
    _i16 status;

    status = osi_SyncObjSignal(&resume_obj);
    OSI_ASSERT_ON_ERROR(status);

    return status;
}


_i16 programmer_pause(void) {
    return 0;
}


static void prog_task(void *pvParameters) {
prog_entry:
    osi_SyncObjWait(&resume_obj, OSI_WAIT_FOREVER);

    /* Here comes the action */
    spi_enable();
}

