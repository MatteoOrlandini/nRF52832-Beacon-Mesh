/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>
#include <string.h>

/* HAL */
#include "boards.h"
#include "app_timer.h"

/* Core */
#include "nrf_mesh_configure.h"
#include "nrf_mesh.h"
#include "mesh_stack.h"
#include "device_state_manager.h"
#include "access_config.h"
#include "nrf_sdh_soc.h"

/* Provisioning and configuration */
#include "mesh_provisionee.h"
#include "mesh_app_utils.h"

/* Models */
#include "generic_onoff_client.h"

/* Logging and RTT */
#include "nrf_log.h"

/* Example specific includes */
#include "app_config.h"
#include "nrf_mesh_config_examples.h"
#include "light_switch_example_common.h"
#include "example_common.h"

/* advertiser.h defines advertiser_t variable */
#include "advertiser.h"

#define APP_STATE_OFF                (0)
#define APP_STATE_ON                 (1)

#define APP_UNACK_MSG_REPEAT_COUNT   (2)

#define MESH_SOC_OBSERVER_PRIO  0

/* Beacon define */
#define ADVERTISER_BUFFER_SIZE  (64)

/* Defines for iBeacon packet */
#define APP_FLAG_BEACON_LENGTH          0x02                               /**< Length of the discoverability Beacon. */
#define APP_ADVERTISER_LENGTH           0x1A                               /**< Total length of the iBeacon. */
#define APP_iBEACON_LENGTH              0x15                               /**< Length of manufacturer specific data in the advertisement. */
#define APP_iBEACON_TYPE                0x02                               /**< iBeacon Type. */ 
#define APP_MEASURED_RSSI               0xC3                               /**< The Beacon's measured RSSI at 1 meter distance in dBm. */
#define APP_COMPANY_IDENTIFIER_NORDIC   0x59, 0x00                         /**< Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org. */
#define APP_COMPANY_IDENTIFIER_APPLE    0x4C, 0x00                         /**< Apple company identifier. */
#define APP_MAJOR_VALUE                 0x01, 0x02                         /**< Major value used to identify Beacons. */
#define APP_MINOR_VALUE                 0x03, 0x04                         /**< Minor value used to identify Beacons. */
#define APP_BEACON_UUID                 0x01, 0x12, 0x23, 0x34, \
                                        0x45, 0x56, 0x67, 0x78, \
                                        0x89, 0x9A, 0xAB, 0xBC, \
                                        0xCD, 0xDE, 0xEF, 0xF0            /**< Proprietary UUID for Beacon. */

/*Defines for Eddystone packet */
#define APP_SERVICE_UUID_LENGTH         0x03                              /**< Complete list of UUID service advertising data length. */
#define APP_EDDYSTONE_DATA_LENGTH       0x15                              /**< Eddystone Frame Length. */
#define APP_EDDYSTONE_UUID              0xAA, 0xFE                        /**< Eddystone UUID. */
#define APP_EDDYSTONE_URL_FRAME_TYPE    0x10                              /**< Eddystone URL Frame Type. */
#define APP_URL_PREFIX                  0x00                              /**< Eddystone URL Prefix. */

/* Eddystone macro */ 
#define DEAD_BEEF                       0xDEADBEEF          //!< Value used as error code on stack dump, can be used to identify stack location on stack unwind.
#define NON_CONNECTABLE_ADV_LED_PIN     BSP_BOARD_LED_0     //!< Toggles when non-connectable advertisement is sent.
#define CONNECTED_LED_PIN               BSP_BOARD_LED_1     //!< Is on when device has connected.
#define CONNECTABLE_ADV_LED_PIN         BSP_BOARD_LED_2     //!< Is on when device is advertising connectable advertisements.


/* Defines for Eddystone, iBeacon andComplete Local Name Beacon */
#undef  EddystoneURL
#define  iBeaconAndroid
#define  iBeaconApple

static generic_onoff_client_t m_clients[CLIENT_MODEL_INSTANCE_COUNT];
static bool                   m_device_provisioned;

/** Single advertiser instance. May periodically transmit one packet at a time. */
static advertiser_t m_advertiser_ibeacon_android;
static advertiser_t m_advertiser_ibeacon_apple;
static advertiser_t m_advertiser_eddystone;

static uint8_t      m_adv_buffer_iBeaconAndroid[ADVERTISER_BUFFER_SIZE];
static uint8_t      m_adv_buffer_iBeaconApple[ADVERTISER_BUFFER_SIZE];
static uint8_t      m_adv_buffer_eddystone[ADVERTISER_BUFFER_SIZE];

/* Forward declaration */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self);
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in);
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status);

const generic_onoff_client_callbacks_t client_cbs =
{
    .onoff_status_cb = app_generic_onoff_client_status_cb,
    .ack_transaction_status_cb = app_gen_onoff_client_transaction_status_cb,
    .periodic_publish_cb = app_gen_onoff_client_publish_interval_cb
};

static void mesh_soc_evt_handler(uint32_t evt_id, void * p_context)
{
    nrf_mesh_on_sd_evt(evt_id);
}

NRF_SDH_SOC_OBSERVER(m_mesh_soc_observer, MESH_SOC_OBSERVER_PRIO, mesh_soc_evt_handler, NULL);

static void provisioning_complete_cb(void)
{
    NRF_LOG_INFO("Successfully provisioned\n");

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    NRF_LOG_INFO("Node Address: 0x%04x \n", node_address.address_start);
}

/* This callback is called periodically if model is configured for periodic publishing */
static void app_gen_onoff_client_publish_interval_cb(access_model_handle_t handle, void * p_self)
{
    NRF_LOG_WARNING("Publish desired message here.\n");
}

/* Acknowledged transaction status callback, if acknowledged transfer fails, application can
* determine suitable course of action (e.g. re-initiate previous transaction) by using this
* callback.
*/
static void app_gen_onoff_client_transaction_status_cb(access_model_handle_t model_handle,
                                                       void * p_args,
                                                       access_reliable_status_t status)
{
    switch(status)
    {
        case ACCESS_RELIABLE_TRANSFER_SUCCESS:
            NRF_LOG_INFO("Acknowledged transfer success.\n");
            break;

        case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
            NRF_LOG_INFO("Acknowledged transfer timeout.\n");
            break;

        case ACCESS_RELIABLE_TRANSFER_CANCELLED:
            NRF_LOG_INFO("Acknowledged transfer cancelled.\n");
            break;

        default:
            ERROR_CHECK(NRF_ERROR_INTERNAL);
            break;
    }
}

/* Generic OnOff client model interface: Process the received status message in this callback */
static void app_generic_onoff_client_status_cb(const generic_onoff_client_t * p_self,
                                               const access_message_rx_meta_t * p_meta,
                                               const generic_onoff_status_params_t * p_in)
{
    if (p_in->remaining_time_ms > 0)
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OnOff server: 0x%04x, Present OnOff: %d, Target OnOff: %d, Remaining Time: %d ms\n",
              p_meta->src.value, p_in->present_on_off, p_in->target_on_off, p_in->remaining_time_ms);
    }
    else
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OnOff server: 0x%04x, Present OnOff: %d\n",
              p_meta->src.value, p_in->present_on_off);
    }
}

static void node_reset(void)
{
    NRF_LOG_INFO("----- Node reset  -----\n");
    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

static void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

void mesh_main_button_event_handler(uint32_t button_number)
{
    NRF_LOG_INFO("Button %u pressed\n", button_number);

    uint32_t status = NRF_SUCCESS;
    generic_onoff_set_params_t set_params;
    model_transition_t transition_params;
    static uint8_t tid = 0;

    /* Button 1: On, Button 2: Off, Client[0]
     * Button 2: On, Button 3: Off, Client[1]
     */

    switch(button_number)
    {
        case 0:
        case 2:
            set_params.on_off = APP_STATE_ON;
            break;

        case 1:
        case 3:
            set_params.on_off = APP_STATE_OFF;
            break;
    }

    set_params.tid = tid++;
    transition_params.delay_ms = APP_CONFIG_ONOFF_DELAY_MS;
    transition_params.transition_time_ms = APP_CONFIG_ONOFF_TRANSITION_TIME_MS;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sending msg: ONOFF SET %d\n", set_params.on_off);

    switch (button_number)
    {
        case 0:
        case 1:
            /* Demonstrate acknowledged transaction, using 1st client model instance */
            /* In this examples, users will not be blocked if the model is busy */
            (void)access_model_reliable_cancel(m_clients[0].model_handle);
            status = generic_onoff_client_set(&m_clients[0], &set_params, &transition_params);
            break;

        case 2:
        case 3:
            /* Demonstrate un-acknowledged transaction, using 2nd client model instance */
            status = generic_onoff_client_set_unack(&m_clients[1], &set_params,
                                                    &transition_params, APP_UNACK_MSG_REPEAT_COUNT);
            break;
    }

    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_NO_MEM:
        case NRF_ERROR_BUSY:
        case NRF_ERROR_INVALID_STATE:
            NRF_LOG_INFO("Client %u cannot send\n", button_number);
            break;

        case NRF_ERROR_INVALID_PARAM:
            /* Publication not enabled for this client. One (or more) of the following is wrong:
             * - An application key is missing, or there is no application key bound to the model
             * - The client does not have its publication state set
             *
             * It is the provisioner that adds an application key, binds it to the model and sets
             * the model's publication state.
             */
            NRF_LOG_INFO("Publication not configured for client %u\n", button_number);
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void models_init_cb(void)
{
    NRF_LOG_INFO("Initializing and adding models\n");

    for (uint32_t i = 0; i < CLIENT_MODEL_INSTANCE_COUNT; ++i)
    {
        m_clients[i].settings.p_callbacks = &client_cbs;
        m_clients[i].settings.timeout = 0;
        m_clients[i].settings.force_segmented = APP_CONFIG_FORCE_SEGMENTATION;
        m_clients[i].settings.transmic_size = APP_CONFIG_MIC_SIZE;

        ERROR_CHECK(generic_onoff_client_init(&m_clients[i], i + 1));
    }
}

/* Beaconing functions */
static void rx_cb(const nrf_mesh_adv_packet_rx_data_t * p_rx_data)
{
    //LEDS_OFF(BSP_LED_1_MASK);  /* @c LED_RGB_RED_MASK on pca10031 */
    char msg[128];
    (void) sprintf(msg, "RX [@%u]: RSSI: %3d ADV TYPE: 0x%02x CHANNEL %d ADDR: [%02x:%02x:%02x:%02x:%02x:%02x]",
                   p_rx_data->p_metadata->params.scanner.timestamp,
                   p_rx_data->p_metadata->params.scanner.rssi,
                   p_rx_data->adv_type,
                   p_rx_data->p_metadata->params.scanner.channel,
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[0],
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[1],
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[2],
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[3],
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[4],
                   p_rx_data->p_metadata->params.scanner.adv_addr.addr[5]);
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, msg, p_rx_data->p_payload, p_rx_data->length);
    //LEDS_ON(BSP_LED_1_MASK);  /* @c LED_RGB_RED_MASK on pca10031 */
}

static void adv_init(void)
{
    #ifdef iBeaconAndroid
      advertiser_instance_init(&m_advertiser_ibeacon_android, NULL, m_adv_buffer_iBeaconAndroid, ADVERTISER_BUFFER_SIZE);
    #endif

    #ifdef iBeaconApple
      advertiser_instance_init(&m_advertiser_ibeacon_apple, NULL, m_adv_buffer_iBeaconApple, ADVERTISER_BUFFER_SIZE);
    #endif

    #ifdef EddystoneURL
      advertiser_instance_init(&m_advertiser_eddystone, NULL, m_adv_buffer_eddystone, ADVERTISER_BUFFER_SIZE);
    #endif
}

static void adv_start(void)
{
    bearer_adtype_add(BLE_GAP_AD_TYPE_SERVICE_DATA);

#ifdef iBeaconAndroid

    advertiser_enable(&m_advertiser_ibeacon_android);

    static const uint8_t adv_data_iBeaconAndroid[] =
    {
        APP_FLAG_BEACON_LENGTH, /* Flag Beacon length (including type, but not itself) */
        BLE_GAP_AD_TYPE_FLAGS, /**< Flags for discoverability. */
        BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED, /**< Non-connectable scannable undirected advertising events. */
        APP_ADVERTISER_LENGTH, /**< Advertiser data length. */
        BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, /**< Manufacturer Specific Data. */
        APP_COMPANY_IDENTIFIER_NORDIC, /* Company ID */
        APP_iBEACON_TYPE, /* iBeacon Type */
        APP_iBEACON_LENGTH, /* iBeacon Lenght */
        APP_BEACON_UUID,  /* UUID */
        APP_MAJOR_VALUE, /* Major Value*/
        APP_MINOR_VALUE, /* Minor Value */
        APP_MEASURED_RSSI  /* RSSI at 1 m */
    };

    /* Allocate packet */
    adv_packet_t * p_packet_ibeacon_android = advertiser_packet_alloc(&m_advertiser_ibeacon_android, sizeof(adv_data_iBeaconAndroid));
    if (p_packet_ibeacon_android)
    {
        /* Construct packet contents */
        memcpy(p_packet_ibeacon_android->packet.payload, adv_data_iBeaconAndroid, sizeof(adv_data_iBeaconAndroid));
        /* Repeat forever */
        p_packet_ibeacon_android->config.repeats = ADVERTISER_REPEAT_INFINITE;

        advertiser_packet_send(&m_advertiser_ibeacon_android, p_packet_ibeacon_android);
    }
#endif

#ifdef iBeaconApple

    advertiser_enable(&m_advertiser_ibeacon_apple);

    static const uint8_t adv_data_iBeaconApple[] =
    {
        APP_FLAG_BEACON_LENGTH, /* Flag Beacon length (including type, but not itself) */
        BLE_GAP_AD_TYPE_FLAGS, /**< Flags for discoverability. */
        BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED, /**< Non-connectable scannable undirected advertising events. */
        APP_ADVERTISER_LENGTH, /**< Advertiser data length. */
        BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, /**< Manufacturer Specific Data. */
        APP_COMPANY_IDENTIFIER_APPLE, /* Company ID */
        APP_iBEACON_TYPE, /* iBeacon Type */
        APP_iBEACON_LENGTH, /* iBeacon Lenght */
        APP_BEACON_UUID,  /* UUID */
        APP_MAJOR_VALUE, /* Major Value*/
        APP_MINOR_VALUE, /* Minor Value */
        APP_MEASURED_RSSI  /* RSSI at 1 m */
    };

    /* Allocate packet */
    adv_packet_t * p_packet_ibeacon_apple = advertiser_packet_alloc(&m_advertiser_ibeacon_apple, sizeof(adv_data_iBeaconApple));
    if (p_packet_ibeacon_apple)
    {
        /* Construct packet contents */
        memcpy(p_packet_ibeacon_apple->packet.payload, adv_data_iBeaconApple, sizeof(adv_data_iBeaconApple));
        /* Repeat forever */
        p_packet_ibeacon_apple->config.repeats = ADVERTISER_REPEAT_INFINITE;

        advertiser_packet_send(&m_advertiser_ibeacon_apple, p_packet_ibeacon_apple);
    }
#endif

#ifdef EddystoneURL

    advertiser_enable(&m_advertiser_eddystone);

    static const uint8_t adv_data_eddystone[] =
    {
        APP_FLAG_BEACON_LENGTH, /* Flag Beacon length (including type, but not itself) */
        BLE_GAP_AD_TYPE_FLAGS, /**< Flags for discoverability. */
        BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED, /**< Non-connectable scannable undirected advertising events. */
        APP_SERVICE_UUID_LENGTH, /**< Server UUID advertising data length */
        BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,  /**< Complete list of 16 bit service UUIDs. */
        APP_EDDYSTONE_UUID, /**< Eddystone UUID. */
        APP_EDDYSTONE_DATA_LENGTH, /**< Eddystone data length. */
        BLE_GAP_AD_TYPE_SERVICE_DATA, /**< Service Data - 16-bit UUID. */
        APP_EDDYSTONE_UUID, /**< Eddystone UUID. */
        APP_EDDYSTONE_URL_FRAME_TYPE, /**< Frame Type: Eddystone URL. */
        APP_MEASURED_RSSI, /**< Ranging data. */
        APP_URL_PREFIX, /**< URL prefix http://www. */
        'n', /**< URL. */
        'o',
        'r',
        'd',
        'i',
        'c',
        's',
        'e',
        'm',
        'i',
        '.',
        'c',
        'o',
        'm',
        '/'
    };
    
    /* Allocate packet */
    adv_packet_t * p_packet_eddystone = advertiser_packet_alloc(&m_advertiser_eddystone, sizeof(adv_data_eddystone));
    if (p_packet_eddystone)
    {
        /* Construct packet contents */
        memcpy(p_packet_eddystone->packet.payload, adv_data_eddystone, sizeof(adv_data_eddystone));

        /* Repeat forever */
        p_packet_eddystone->config.repeats = ADVERTISER_REPEAT_INFINITE;

        advertiser_packet_send(&m_advertiser_eddystone, p_packet_eddystone);
    }
#endif
}

void mesh_init(void)
{
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = NULL,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };

    uint32_t status = mesh_stack_init(&init_params, &m_device_provisioned);
    switch (status)
    {
        case NRF_ERROR_INVALID_DATA:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data in the persistent memory was corrupted. Device starts as unprovisioned.\n");
            break;
        case NRF_SUCCESS:
            break;
        default:
            ERROR_CHECK(status);
    }

    /* Start listening for incoming packets */
    //nrf_mesh_rx_cb_set(rx_cb);

    /* Initialize the advertiser */
    adv_init();
}

void mesh_main_initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);

    mesh_init();
}

void mesh_main_start(void)
{
    if (!m_device_provisioned)
    {
        static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
        mesh_provisionee_start_params_t prov_start_params =
        {
            .p_static_data    = static_auth_data,
            .prov_complete_cb = provisioning_complete_cb,
            .prov_device_identification_start_cb = NULL,
            .prov_device_identification_stop_cb = NULL,
            .prov_abort_cb = NULL,
            .p_device_uri = EX_URI_LS_CLIENT
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }

    /* Start advertising own beacon */
    /* Note: If application wants to start beacons at later time, adv_start() API must be called
     * from the same IRQ priority context same as that of the Mesh Stack. */
    adv_start();

    const uint8_t *p_uuid = nrf_mesh_configure_device_uuid_get();
    NRF_LOG_INFO("Device UUID");
    NRF_LOG_RAW_HEXDUMP_INFO(p_uuid, NRF_MESH_UUID_SIZE);

    ERROR_CHECK(mesh_stack_start());
}
