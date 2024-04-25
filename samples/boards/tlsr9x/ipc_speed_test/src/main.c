/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "main.h"


static uint8_t tx_data[IPC_TX_DATA_BUFF_SIZE] = {[0 ... (SEND_DATA_SIZE_MAX-1)] = 0xFF};
static uint8_t tx_buffer[RECEIVE_BUFF_SIZE] = {[0 ... (RECEIVE_BUFF_SIZE-1)] = 0x0};

size_t length [] = {10, 20, 30};

static const uint16_t tx_data_len[SEND_DATA_LEN_TOT] = 
{
    [SEND_DATA_LEN_16B]  = SEND_DATA_16B,
    [SEND_DATA_LEN_32B]  = SEND_DATA_32B,
    [SEND_DATA_LEN_64B]  = SEND_DATA_64B,
    [SEND_DATA_LEN_128B] = SEND_DATA_128B,
    [SEND_DATA_LEN_256B] = SEND_DATA_256B,
    [SEND_DATA_LEN_512B] = SEND_DATA_512B,
    [SEND_DATA_LEN_1K]   = SEND_DATA_1KB
};

static struct ipc_based_driver ipc_drv;    /* ipc driver part */
static uint64_t sys_time = 0;

static void clear_rx_buffer_content(void)
{
    for (uint16_t i = 0; i < RECEIVE_BUFF_SIZE; i++)
    {
        tx_buffer[i] = 0;
    }
}

static const char* check_response_data(
    ipc_data_tx_t* ipc_respond_ptr, uint16_t data_resp_len, uint8_t correct_val)
{
    const char* ret_ok   = "SUCCESS";
    const char* ret_fail = "FAIL";
    char*       result   = (char*)ret_ok;
    uint16_t i = 0;

    for (i = 0; i < data_resp_len; i++)
    {
        if (ipc_respond_ptr->data_arr[i] != correct_val)
        {
            result = (char*)ret_fail;
            break;
        }
    }

    return (const char*)result;
}

static void ipc_init_test(void)
{
    printk("Init IPC Driver\n\r");

    ipc_based_driver_init(&ipc_drv);
}

static size_t pack_ipc_send_data_test(
    uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
    uint32_t id = 0;
    ipc_data_tx_t* ipc_data_tx = unpack_data;
    size_t pack_data_len = sizeof(id) + sizeof(ipc_data_tx->error) +
        sizeof(ipc_data_tx->data_len) + ipc_data_tx->data_len;

    if (pack_data != NULL)
    {
        id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_TRNG_GET_TEST, inst);

        IPC_DISPATCHER_PACK_FIELD(pack_data, id);
        IPC_DISPATCHER_PACK_FIELD(pack_data, ipc_data_tx->error);
        IPC_DISPATCHER_PACK_FIELD(pack_data, ipc_data_tx->data_len);
        IPC_DISPATCHER_PACK_ARRAY(pack_data, ipc_data_tx->data_arr, 
                                  ipc_data_tx->data_len);
    }

    // Detect start of transfering data from D25 to N22 core
    gpio_set_output(RED_LED_PIN, PIN_STATE_ON);
    // Save start measure time
    sys_time = sys_clock_cycle_get_64();

    return pack_data_len;
}

static void unpack_ipc_send_data_test(
    void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
    // Detect end of transfering data from N22 to D25 core
    sys_time = sys_clock_cycle_get_64() - sys_time;
    // Clear LED inputs to Low level
    gpio_set_output(RED_LED_PIN, PIN_STATE_OFF);
    gpio_set_output(GREEN_LED_PIN, PIN_STATE_OFF);

    ipc_data_tx_t* ipc_data_rx = unpack_data;
    size_t expected_len = sizeof(uint32_t) + sizeof(ipc_data_rx->error) +
        sizeof(ipc_data_rx->data_len) + ipc_data_rx->data_len;

    pack_data += sizeof(uint32_t);

    IPC_DISPATCHER_UNPACK_FIELD(pack_data, ipc_data_rx->error);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, ipc_data_rx->data_len);

    if (expected_len == pack_data_len)
    {
        IPC_DISPATCHER_UNPACK_ARRAY(pack_data, ipc_data_rx->data_arr,
            ipc_data_rx->data_len);
	}
    else
    {
        printk("Invalid RX length (exp %d/ got %d)",
                expected_len, pack_data_len);
    }
}

static uint32_t ipc_send_data_test(
   struct ipc_based_driver *ipc_drv, uint8_t* tx_buffer,
   uint16_t buff_len, uint16_t error, ipc_data_tx_t* ipc_respond
)
{
    uint32_t measure_time_us = 0;
    uint8_t  count           = 0;             

    ipc_data_tx_t ipc_data_tx = 
    {
        .error    = error, 
        .data_len = buff_len,
        .data_arr = tx_buffer
    };

    for (count = 0; count < IPC_SEND_DATA_TIMES; count++)
    {
        clear_rx_buffer_content();
        // // Detect start of transfering data from D25 to N22 core
        // gpio_set_output(RED_LED_PIN, PIN_STATE_ON);
        // // Save start measure time
        // sys_time = sys_clock_cycle_get_64();
        IPC_DISPATCHER_HOST_SEND_DATA(ipc_drv, IPC_DEV_INSTANCE, ipc_send_data_test,
                                      &ipc_data_tx, ipc_respond, 2000);

        // // Detect end of transfering data from N22 to D25 core
        // sys_time = sys_clock_cycle_get_64() - sys_time;
        // // Clear LED inputs to Low level
        // gpio_set_output(RED_LED_PIN, PIN_STATE_OFF);
        // gpio_set_output(GREEN_LED_PIN, PIN_STATE_OFF);
    
        if (ipc_respond->error == 0)
        {
            measure_time_us += (uint32_t)(sys_time / SYS_TICK_TO_US_DIV);
            k_msleep(SLEEP_TIME_10_MS);
        }
        else
        {
            measure_time_us = 0;
            break;
        }
    }

    if (measure_time_us != 0)
    {
        measure_time_us /= IPC_SEND_DATA_TIMES;
    }

    return measure_time_us;
}

static void gpio_update_reg(uint32_t addr, uint32_t value, bool set)
{
        uint32_t reg = readl(addr);
        if (set) {
                reg |= value;
        } else {
                reg &= ~value;
        }
        writel(reg, addr);
}

void gpio_enable_output(uint8_t gpio)
{
        uint8_t bank  = gpio / 8;
        uint32_t addr  = IOMUX_BASE_ADDR + bank * 4;
        uint8_t shift = (gpio - bank * 8) * 4;

        gpio_update_reg(addr, 0xf << shift, false);
        gpio_update_reg(addr, 0x8 << shift, true);

        gpio_update_reg(GPIO_BASE_ADDR + 0x0c, 1 << gpio, false);
        gpio_update_reg(GPIO_BASE_ADDR + 0x10, 1 << gpio, true);
}

void gpio_set_output(uint8_t gpio, bool state)
{
        gpio_update_reg(GPIO_BASE_ADDR + 0x14, 1 << gpio, state);
}


typedef struct
{
    uint32_t result_op;
    uint32_t measure_time_us;
    uint8_t* tx_data_buf;
} ipc_data_send_ctx_t;

typedef struct 
{
	struct k_sem *resp_sem;
	struct ipc_data_send_ctx_t *ctx;
} ipc_response_data_t;

static void resp_cb(const void *data, size_t len, void *param)
{
	ipc_response_data_t* response_data = (ipc_response_data_t*)param;
    uint8_t* received_data = (uint8_t*)data;
	// if (response_data->ctx && response_data->ctx->unpack) {
	// 	response_data->ctx->unpack(response_data->ctx->result, data, len);
	// }
    for (uint16_t i = 0; i < len; i++)
    {
        printk("data %d\n\r", received_data[i]);    
    }
    printk("IPC data CB\n\r");
	k_sem_give(response_data->resp_sem);
}

static int ipc_data_send(
    struct ipc_based_driver *drv, uint8_t* tx_data_ptr,
    uint16_t tx_len, ipc_data_send_ctx_t *ctx, uint32_t timeout_ms)
{
	int result = 0;
    uint8_t* tx_data_start_ptr = (uint8_t*)tx_data_ptr;
    uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_TRNG_GET_TEST, IPC_DEV_INSTANCE);
    uint16_t error = 0;
    uint16_t data_len = tx_len;
    size_t pack_data_len = sizeof(id) + sizeof(error) +
        sizeof(data_len) + (size_t)data_len;

	ipc_response_data_t response_data = {
		.resp_sem = &drv->resp_sem,
		.ctx = ctx,
	};
    // Fill fields in the packet
    IPC_DISPATCHER_PACK_FIELD(tx_data_ptr, id);
    IPC_DISPATCHER_PACK_FIELD(tx_data_ptr, error);
    IPC_DISPATCHER_PACK_FIELD(tx_data_ptr, data_len);
    IPC_DISPATCHER_PACK_ARRAY(tx_data_ptr, tx_data, tx_len);

	k_mutex_lock(&drv->mutex, K_FOREVER);

	k_sem_reset(&drv->resp_sem);
	ipc_dispatcher_add(id, resp_cb, &response_data);

	do {
		if (ipc_dispatcher_send(tx_data_start_ptr, pack_data_len) != pack_data_len) {
			printk("IPC data send error (id=%x)", id);
			result = -ECANCELED;
			break;
		}

		if (k_sem_take(&drv->resp_sem, K_MSEC(timeout_ms))) {
			printk("IPC response timeout (id=%x)", id);
			result = -ETIMEDOUT;
			break;
		}
	} while (0);

	ipc_dispatcher_rm(id);
	k_mutex_unlock(&drv->mutex);

	return result;
}

int main(void)
{
    uint32_t measure_time_us = 0;
    uint8_t  i               = 0;
    uint8_t  temp            = 0; 
    bool     flag            = PIN_STATE_OFF;

    ipc_data_tx_t ipc_respond =
    {
        .error    = 0,
        .data_len = 0,
        .data_arr = tx_buffer
    };

    ipc_data_send_ctx_t ctx;

    // Enable RED LED output and set it to the low level
    gpio_enable_output(RED_LED_PIN);
    gpio_set_output(RED_LED_PIN, flag);

    ipc_init_test();

    k_msleep(SLEEP_TIME_1_SEC);

    for (i = 0; i < 1/*SEND_DATA_LEN_TOT*/; i++)
    {
        ctx.result_op       = 0;
        ctx.measure_time_us = 0;
        ctx.tx_data_buf     = tx_data + i;
        
        temp = 0;
        printk("Ensure that IPC Respond data is cleared by %d\n\rResult: %s.\n\r",
               temp, check_response_data(&ipc_respond, tx_data_len[i], temp));

        // Strarting transfer data to another core (D25)->(N22)
        printk("Strarting to transfer data: (D25)->(N22)\n\r");
        //printk("Sending %d Bytes of data ", tx_data_len[i]);
        //printk("with %d Bytes packet\n\r", (tx_data_len[i] + SEND_DATA_HEADER_SIZE));

        k_msleep(SLEEP_TIME_100_MS);

        ipc_respond.data_len = tx_data_len[i];

        // measure_time_us = ipc_send_data_test(&ipc_drv, tx_data, tx_data_len[i], 
        //                                      0, &ipc_respond);
        ipc_data_send(&ipc_drv, tx_buffer, tx_data_len[i], &ctx, 100);

        //printk("Data has been sent %d times\n\r", IPC_SEND_DATA_TIMES);
        //printk("Average IPC measure time is: %d us.\n\r", measure_time_us);

        temp = 0xFF;
        printk("Ensure that IPC Respond data is filled by %d\n\rResult: %s.\n\r",
               temp, check_response_data(&ipc_respond, tx_data_len[i], temp));
        printk("\n\r");

        clear_rx_buffer_content();
    }

    // Wait before start blinking
    k_msleep(SLEEP_TIME_5_SEC);
    while (1) {
        flag = (flag) ? PIN_STATE_OFF : PIN_STATE_ON;

        gpio_set_output(RED_LED_PIN, flag);

        k_msleep(SLEEP_TIME_1_SEC);
    }
    return 0;
}
