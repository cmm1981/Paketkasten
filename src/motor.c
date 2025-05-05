/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>
#include "motor.h"

#define MOTORV_PWM_NODE DT_NODELABEL(motor_vor)
#define MOTORZ_PWM_NODE DT_NODELABEL(motor_zur)

#define MOTOR_OFF 0
#define MOTOR_PWM_PERIOD DT_PWMS_PERIOD(MOTORV_PWM_NODE)
#define MOTOR_PWM_PULSE_START MOTOR_PWM_PERIOD/2U

#define MOTOR_ERR_PWM_NOT_READY -1
#define MOTOR_ERR_PWM_SET       -2
#define MOTOR_ERR_ADC_NOT_READY -3
#define MOTOR_ERR_ADC_SETUP     -4
#define MOTOR_ERR_ADC_START     -5

#define MOTOR_ADC_NODE        DT_NODELABEL(adc1)
#define MOTOR_ADC_RESOLUTION  12
#define MOTOR_ADC_CHANNEL     1  // ADC_IN1 → PA1 
#define MOTOR_ADC_BUFFER_SIZE     20
#define MOTOR_ADC_INTERVAL_US     500

#define MOTOR_MAIN_STACK_SIZE 1024
#define MOTOR_MAIN_PRIORITY 5

K_THREAD_STACK_DEFINE(motor_main_stack, MOTOR_MAIN_STACK_SIZE);
static struct k_thread motor_main_data;
k_tid_t motor_main_id;

static const struct pwm_dt_spec motorv = PWM_DT_SPEC_GET(DT_ALIAS(motorv));
static const struct pwm_dt_spec motorz = PWM_DT_SPEC_GET(DT_ALIAS(motorz));

static uint16_t motor_adc_buffer_a[MOTOR_ADC_BUFFER_SIZE];
static uint16_t motor_adc_buffer_b[MOTOR_ADC_BUFFER_SIZE];
static uint16_t *motor_adc_current_buffer = motor_adc_buffer_a;

static struct k_poll_signal motor_adc_done_signal;
static struct k_poll_event motor_adc_event;

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec motor_adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

 struct adc_sequence_options motor_seq_options = {
    .callback = NULL,
    .extra_samplings = MOTOR_ADC_BUFFER_SIZE-1,
    .interval_us = MOTOR_ADC_INTERVAL_US,
    .user_data = NULL,
};

struct adc_sequence motor_sequence = {
    .options     = &motor_seq_options,
    .buffer      = motor_adc_buffer_a,
    .buffer_size = sizeof(motor_adc_buffer_a),
    .calibrate   = false,
};

typedef struct
{
    motor_richtung_t richtung_soll; //Angeforderte Drehrichtung
    uint16_t timeout_10ms;          //Zeit bis zum Timeout pro 10ms (100raw * 10ms = 1s)
    bool *stop;                    //Zeiger zum Stop Kriterium
    uint16_t pulse;
} motor_t;

motor_t motor;

typedef struct
{
    motor_richtung_t richtung_soll; //Angeforderte Drehrichtung
    uint16_t timeout_10ms;          //Zeit bis zum Timeout pro 10ms (100raw * 10ms = 1s)
    bool *stop;                    //Zeiger zum Stop Kriterium
} motor_set_t;

K_PIPE_DEFINE(motor_set_pipe, sizeof(motor_set_t), 4);


/* 10ms Funktion zur Motorregelung */
void motor_main(void *p1, void *p2, void *p3)
{
    int ret;
    motor_set_t my_motor_set;
    uint32_t motor_strom;
    
    while(1)
    {
        /* Auf ADC Werte warte => 10ms Takt erzeugen
            - k_poll kommt ca. alle 10,4ms Zurück */
        ret = k_poll(&motor_adc_event, 1, K_FOREVER);
        if (ret < 0) 
        {
            printk("k_poll error: %d\n", ret);
        }

        k_poll_signal_reset(&motor_adc_done_signal);

        /* Buffer bearbeiten */
        motor_strom = 0;
        for(int i = 0; i < MOTOR_ADC_BUFFER_SIZE; i++)
        {
            motor_strom += motor_adc_current_buffer[i];
        }
        /* Mittelwert bilden */
        motor_strom = motor_strom / MOTOR_ADC_BUFFER_SIZE;

        /* Umrechung in mV*/
        adc_raw_to_millivolts_dt(&motor_adc_channels[0],&motor_strom);

        /* Umrechnung in mA: 0,5 Ohm => I = U/R = U/0,5 = U*2 */
        motor_strom = motor_strom * 2;

        /* Buffer wechseln */
        motor_adc_current_buffer = (motor_adc_current_buffer == motor_adc_buffer_a) ? motor_adc_buffer_b : motor_adc_buffer_a;

        /* Nächste ADC-DMA Übertragung starten */
        (void)adc_sequence_init_dt(&motor_adc_channels[0], &motor_sequence);
        motor_sequence.buffer = motor_adc_current_buffer;
        motor_sequence.buffer_size = sizeof(motor_adc_buffer_a);
        ret = adc_read_async((&motor_adc_channels[0])->dev, &motor_sequence, &motor_adc_done_signal);
        if (ret < 0) {
            printk("ADC async read failed: %d\n", ret);
        }

        /* Checken ob neue Befehl vorliegt*/
        if(k_pipe_read(&motor_set_pipe, (uint8_t *)&my_motor_set, sizeof(motor_set_t), K_NO_WAIT) == sizeof(motor_set_t))
        {
            motor.richtung_soll = my_motor_set.richtung_soll;
            motor.timeout_10ms = my_motor_set.timeout_10ms;
            motor.stop = my_motor_set.stop;
        }

        /* ToDo: Stromregelung*/

        /* Check endstop */
        if(*motor.stop)
        {
            /* Endstop erreicht */
            motor.richtung_soll = MOTOR_STOP;
        }

        /* Rechne Timeout */
        if(motor.richtung_soll != MOTOR_STOP && motor.timeout_10ms > 0)
        {
            motor.timeout_10ms--;
            if(!motor.timeout_10ms)
            {
                printk("Motor Error: Timeout before endstop reached\n");
                motor.richtung_soll = MOTOR_STOP;
            }
        }

        /* Schalte PWM für den Motor */
        switch(motor.richtung_soll)
        {
            case MOTOR_VOR:
                if(pwm_set_dt(&motorv, MOTOR_PWM_PERIOD, motor.pulse))
                {
                    printk("Error: failed to set pulse width\n");
                }
                if(pwm_set_dt(&motorz, MOTOR_PWM_PERIOD, MOTOR_OFF))
                {
                    printk("Error: failed to set pulse width\n");
                }
            break;

            case MOTOR_ZUR:
                if(pwm_set_dt(&motorv, MOTOR_PWM_PERIOD, MOTOR_OFF))
                {
                    printk("Error: failed to set pulse width\n");
                }
                if(pwm_set_dt(&motorz, MOTOR_PWM_PERIOD, motor.pulse))
                {
                    printk("Error: failed to set pulse width\n");
                }
            break;

            default:
                /* Stop Motor */
                if(pwm_set_dt(&motorv, MOTOR_PWM_PERIOD, MOTOR_OFF))
                {
                    printk("Error: failed to set pulse width\n");
                }
                if(pwm_set_dt(&motorz, MOTOR_PWM_PERIOD, MOTOR_OFF))
                {
                    printk("Error: failed to set pulse width\n");
                }
        }
    }
}

/* Initialization of necessary Inputs (ADC) and outputs (PWM) */
int motor_init(void)
{
    int ret;

    motor.richtung_soll = MOTOR_STOP;
    motor.pulse = MOTOR_PWM_PULSE_START;

    if (!pwm_is_ready_dt(&motorv)) {
		printk("Error: PWM device %s is not ready\n",
		       motorv.dev->name);
		return MOTOR_ERR_PWM_NOT_READY;
	}

    if (!pwm_is_ready_dt(&motorz)) {
		printk("Error: PWM device %s is not ready\n",
		       motorz.dev->name);
		return MOTOR_ERR_PWM_NOT_READY;
	}

    ret = pwm_set_dt(&motorv, MOTOR_PWM_PERIOD, MOTOR_OFF);
    if (ret) 
    {
        printk("Error %d: failed to set pulse width\n", ret);
        return MOTOR_ERR_PWM_SET;
    }

    ret = pwm_set_dt(&motorz, MOTOR_PWM_PERIOD, MOTOR_OFF);
    if (ret) 
    {
        printk("Error %d: failed to set pulse width\n", ret);
        return MOTOR_ERR_PWM_SET;
    }

    if (!adc_is_ready_dt(&motor_adc_channels[0])) {
        printk("ADC device not ready!\n");
        return MOTOR_ERR_ADC_NOT_READY;
    }

    /* Init ADC async event handling */
    k_poll_signal_init(&motor_adc_done_signal);
    k_poll_event_init(&motor_adc_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &motor_adc_done_signal);

    ret = adc_channel_setup_dt(&motor_adc_channels[0]);
    if (ret != 0) {
        printk("ADC channel setup failed (%d)\n", ret);
        return MOTOR_ERR_ADC_SETUP;
    }

    /* ADC Read starten */
    (void)adc_sequence_init_dt(&motor_adc_channels[0], &motor_sequence);
    motor_sequence.buffer = motor_adc_current_buffer;
    ret = adc_read_async((&motor_adc_channels[0])->dev, &motor_sequence, &motor_adc_done_signal);
    if (ret != 0) {
        printk("ADC Async Read failed (%d)\n", ret);
        return MOTOR_ERR_ADC_START;
    }

    /* Starte motor_main */
    motor_main_id = k_thread_create(&motor_main_data, motor_main_stack,
                                      K_THREAD_STACK_SIZEOF(motor_main_stack),
                                      motor_main,
                                      NULL, NULL, NULL,
                                      MOTOR_MAIN_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

void motor_set(motor_richtung_t richtung, uint8_t timeout_s, bool *stop)
{
    motor_set_t my_motor_set;
    my_motor_set.richtung_soll = richtung;
    my_motor_set.timeout_10ms = (uint16_t)timeout_s * 100U;
    my_motor_set.stop = stop;

    k_pipe_write(&motor_set_pipe, (uint8_t *)&my_motor_set, sizeof(motor_set_t), K_FOREVER); 
}

