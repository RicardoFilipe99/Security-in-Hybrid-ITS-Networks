/**
 ********************************************************************************
 * @file    secure_hybrid_its_networks.c
 * @brief   Secure Hybrid ITS Networks Application for Unex OBU
 * @note    Based on Unex example applications
 * @author  Ricardo Severino
 *******************************************************************************
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#include "error_code_user.h"
#include "poti_caster_service.h"
#include "itsg5_caster_service.h"
#include "itsmsg_codec.h"
#include "inc/frozen.h"

#include <openssl/sha.h>
//#include <openssl/hmac.h>
#include "hmac_sha256.h"

/*
 *******************************************************************************
 * Macros
 *******************************************************************************
 */

/* Predefined CAM information */
#define CAM_PROTOCOL_VERSION (2) /* ETSI EN 302 637-2 V1.4.1 (2019-04) */
#define CAM_STATION_ID_DEF (168U)
#define CAM_STATION_TYPE_DEF ITS_STATION_SPECIAL_VEHICLE

/** Predefined CAM macro functions
 *  Please correct following macros for getting data from additional INS sensors on host system.
 */
#define CAM_SENSOR_GET_DRIVE_DIRECTION() (0) /* DriveDirection_forward */
#define CAM_SENSOR_GET_VEHICLE_LENGTH_VALUE() (38) /* 0.1 metre. */
#define CAM_SENSOR_GET_VEHICLE_LENGTH_CONF() (0) /* VehicleLengthConfidenceIndication_noTrailerPresent */
#define CAM_SENSOR_GET_VEGICLE_WIDTH_VALUE() (18) /* 0.1 metre. */
#define CAM_SENSOR_GET_LONG_ACCEL_VALUE() (0) /* 0.1 m/s^2. */
#define CAM_SENSOR_GET_LONG_ACCEL_CONF() (102) /* 1 ~ 102 */
#define CAM_SENSOR_GET_CURVATURE_VALUE() (0) /* Curvature, 1 over 30000 meters, (-30000 .. 30001) */
#define CAM_SENSOR_GET_CURVATURE_CONF() (7) /* 0 ~ 7. */
#define CAM_SENSOR_GET_CURVATURE_CONF_CAL_MODE() (2) /* CurvatureCalculationMode_unavailable */
#define CAM_SENSOR_GET_YAW_RATE_VALUE() (0) /* 0,01 degree per second. */
#define CAM_SENSOR_GET_YAW_RATE_CONF() (8) /* YawRateConfidence_unavailable_ITS */

#define IS_CAM_SSP_VALID(x, y) (((x) & (y)) ? true : false)
#define CAM_SSP_LEN (3U)
/* SSP Definitions for Permissions in CAM */
/* Octet Position: 0 , SSP Version control */
/* Octet Position: 1 */
#define CEN_DSRC_TOLLING_ZONE (1 << 7)
#define PUBLIC_TRANSPORT (1 << 6)
#define SPECIAL_TRANSPORT (1 << 5)
#define DANGEROUS_GOODS (1 << 4)
#define ROADWORK (1 << 3)
#define RESCUE (1 << 2)
#define EMERGENCY (1 << 1)
#define SAFETY_CAR (1 << 0)
/* Octet Position: 2 */
#define CLOSED_LANES (1 << 7)
#define REQUEST_FOR_RIGHT_OF_WAY (1 << 6)
#define REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT (1 << 5)
#define NO_PASSING (1 << 4)
#define NO_PASSING_FOR_TRUCKS (1 << 3)
#define SPEEED_LIMIT (1 << 2)

#define STATION_CONFIG_FILE_NAME "config_station.json"

#define ERR_MSG_SZ 256
#define MALLOC(sz) malloc(sz)
#define CALLOC(n, sz) calloc((n), (sz))
#define FREE(ptr) free(ptr)
/* clang-format on */

/*
 *******************************************************************************
 * Data type definition
 *******************************************************************************
 */

/* Thread type is using for application send and receive thread, the application thread type is an optional method depend on execute platform */
typedef enum app_thread_type {
    APP_THREAD_TX = 0,
    APP_THREAD_RX = 1
} app_thread_type_t;

/* ITS-S type, defined in ETSI EN 302 636-4-1 V1.2.1 chapter 6.3 Fields of the GeoNetworking address */
typedef enum its_station_type {
    ITS_STATION_UNKNOWN = 0,
    ITS_STATION_PEDESTRAIN = 1,
    ITS_STATION_CYCLIST = 2,
    ITS_STATION_MOPED = 3,
    ITS_STATION_MOTORCYCLE = 4,
    ITS_STATION_PASSENGER_CAR = 5,
    ITS_STATION_BUS = 6,
    ITS_STATION_LIGHT_TRUCK = 7,
    ITS_STATION_HEAVY_TRUCK = 8,
    ITS_STATION_TRAILER = 9,
    ITS_STATION_SPECIAL_VEHICLE = 10,
    ITS_STATION_TRAM = 11,
    ITS_STATION_ROAD_SIDE_UNIT = 15,
} its_station_type_t;

typedef struct station_config {
    uint32_t stationID;
    VehicleRole role;  ///<  Reference to VehicleRole
    bool leftTurnSignalOn;  ///< Left turn signal status, 0 for signal off, 1 for signal on
    bool rightTurnSignalOn;  ///< Right turn signal status, 0 for signal off, 1 for signal on
    bool lightBarInUse;  ///<  Role type: emergency, please refer to LightBarSirenInUse
    bool sirenInUse;  ///<  Role type: emergency, please refer to LightBarSirenInUse
    int32_t causeCode;  ///<  Role type: emergency, please refer to CauseCodeType, -1 for this option not used
} station_config_t;

typedef struct process_message_struct{
    size_t len;
    itsg5_rx_info_t rx_info;
    uint8_t rx_buf[ITSG5_CASTER_PKT_SIZE_MAX];
    long long actTime;
    itsg5_caster_handler_t handler_tx;
};


/*
 *******************************************************************************
 * Global variables
 *******************************************************************************
 */
station_config_t station_config;
bool app_running = true;
char files_path[1024];
char aux_path[1024];
char current_path[1024];
FILE *tx_log_file;
FILE *rx_log_file;
FILE *status_log_file;

FILE *latency;
FILE *latency2;

int tx_counter;
int rx_counter;
int rx_lat_counter;
float avgLatency;

bool is_secured_lat;
int operation_mode;
int security_protocol;
int evaluation_mode;

long long timestamps[1000];

/*
 *******************************************************************************
 * Functions declaration
 *******************************************************************************
 */
static void cam_send(itsg5_caster_comm_config_t *config, bool is_secured, int security_protocol, int evaluation_mode);
static void cam_recv(itsg5_caster_comm_config_t *config, int security_protocol, int evaluation_mode);
static void cam_encode(uint8_t **tx_buf, int *tx_buf_len, poti_fix_data_t *p_fix_data, int security_protocol, int evaluation_mode);
static int cam_decode(uint8_t *rx_buf, int rx_buf_len, itsg5_rx_info_t *p_itsg5_rx_info, int security_protocol, int evaluation_mode);
static int cam_check_msg_permission(CAM_V2 *p_cam_msg, uint8_t *p_ssp, uint8_t ssp_len);
static void dump_mem(void *data, int len);
static void dump_rx_info(itsg5_rx_info_t *rx_info);
static void set_tx_info(itsg5_tx_info_t *tx_info, bool is_secured);
static void cam_recv_lat(itsg5_caster_comm_config_t *rx_config, itsg5_caster_comm_config_t *tx_config);
static void cam_send_lat(itsg5_caster_comm_config_t *config);
static int check_source(uint8_t *p_rx_payload, int rx_payload_len, itsg5_rx_info_t *p_itsg5_rx_info);

static int32_t cam_set_semi_axis_length(double meter);
static int32_t cam_set_heading_value(double degree);
static int32_t cam_set_altitude_confidence(double metre);
static int32_t cam_set_heading_confidence(double degree);
static int32_t cam_set_speed_confidence(double meter_per_sec);

static int load_station_config(station_config_t *config);
static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority);

const char* getCurrentTime();
static long long getTimestamp();
void * processMessage(void *args);

/*
 *******************************************************************************
 * Functions
 *******************************************************************************
 */
void app_signal_handler(int sig_num)
{
    if (sig_num == SIGINT) {
        printf("SIGINT signal!\n");
    }
    if (sig_num == SIGTERM) {
        printf("SIGTERM signal!\n");
    }
    app_running = false;
}

char app_sigaltstack[SIGSTKSZ];
int app_setup_signals(void)
{
    stack_t sigstack;
    struct sigaction sa;
    int ret = -1;

    sigstack.ss_sp = app_sigaltstack;
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    if (sigaltstack(&sigstack, NULL) == -1) {
        perror("signalstack()");
        goto END;
    }

    sa.sa_handler = app_signal_handler;
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(SIGINT, &sa, 0) != 0) {
        perror("sigaction()");
        goto END;
    }
    if (sigaction(SIGTERM, &sa, 0) != 0) {
        perror("sigaction()");
        goto END;
    }

    ret = 0;
END:
    return ret;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    int ret_tx;
    int ret_tx_echo;
    int ret_rx;
    bool is_secured;
    ITSMsgConfig cfg;

    itsg5_app_thread_config_t app_thread_config_tx;
    itsg5_app_thread_config_t app_thread_config_tx_echo;
    itsg5_app_thread_config_t app_thread_config_rx;

    if (argc != 6) {
        printf("v2xcast_sdk_cam_example <IP_of_V2Xcast_service> <is_send> <security mode off:0, on:1> <security protocol off:0, dlapp:1. mfspv:2> <eval mode off:0, securityCT:1, totalCT:2>\n");
        return -1;
    }

    ret_tx = app_setup_signals();
    if (!IS_SUCCESS(ret_tx)) {
        printf("Fail to app_setup_signals\n");
        return -1;
    }

    ret_rx = app_setup_signals();
    if (!IS_SUCCESS(ret_rx)) {
        printf("Fail to app_setup_signals\n");
        return -1;
    }
    is_secured = atoi(argv[3]);
    is_secured_lat = atoi(argv[3]);

    security_protocol = atoi(argv[4]);
    evaluation_mode = atoi(argv[5]);

    if(security_protocol == 1) {
        printf("Using DLAPP Security Protocol\n");
    } else if(security_protocol == 2) {
        printf("Using MFSPV Security Protocol\n");
    } else if(security_protocol == 0) {
        printf("Not using security\n");
    } else {
        printf("Unknown security mode, exiting.\n");
        return -1;
    }

    if(evaluation_mode == 1) {
        printf("Using Security computation time (securityCT) evaluation mode\n");
    } else if(evaluation_mode == 2) {
        printf("Using Total computation time (totalCT) evaluation mode\n");
    } else if(evaluation_mode == 0) {
        printf("Not using an evaluation mode\n");
    } else {
        printf("Unknown evaluation mode, exiting.\n");
        return -1;
    }

    printf("\n\n\n");

    itsg5_caster_comm_config_t config_tx;
    itsg5_caster_comm_config_t config_tx_echo;
    itsg5_caster_comm_config_t config_rx;

    ret_tx = itsmsg_init(&cfg);
    if (!IS_SUCCESS(ret_tx)) {
        printf("Fail to init ITS message\n");
        return -1;
    }

    ret_rx = itsmsg_init(&cfg);
    if (!IS_SUCCESS(ret_rx)) {
        printf("Fail to init ITS message\n");
        return -1;
    }

    const char* currentTime = getCurrentTime();

    snprintf(files_path, sizeof(files_path), "log/%s", currentTime);
    snprintf(current_path, sizeof(current_path), "log/current");

    strcpy(aux_path, files_path);

    mkdir(files_path, 0777);
    status_log_file = fopen(strcat(aux_path, "/status.log"), "a");
    strcpy(aux_path, files_path);

    printf("Starting time: %s\n", currentTime);
    fprintf(status_log_file, "Application Starting time: %s\n", currentTime);

    if(status_log_file == NULL) printf("Error creating status file!");    
    operation_mode = atoi(argv[2]);    
        
    if (operation_mode == 0) { /* Receiving CAMs */

        itsg5_caster_init();

        // Load station setting from configuration file 
        ret_rx = load_station_config(&station_config);
        if (!IS_SUCCESS(ret_rx)) {
            printf("Failed to load station config, ret:%d!\n", ret_rx);
            fprintf(status_log_file, "Failed to load station config, ret:%d!\n", ret_rx);
            return -1;
        }
        config_rx.ip = argv[1];
        config_rx.caster_id = 0;
        config_rx.caster_comm_mode = ITSG5_CASTER_MODE_RX;

        // If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority
        // If the example is run on other platforms, it is optional to set tx and rx message threads name and priority
        itsg5_get_app_thread_config(&app_thread_config_rx);
        ret_rx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, app_thread_config_rx.rx_thread_name, app_thread_config_rx.rx_thread_priority_low);

        rx_log_file = fopen(strcat(aux_path, "/rx.log"), "a");
        strcpy(aux_path, files_path);
        if(rx_log_file == NULL) printf("Error creating status file!"); 

        rx_counter = 0;
        cam_recv(&config_rx, security_protocol, evaluation_mode);

        fprintf(rx_log_file, "Received %d messages\n", rx_counter);
        fclose(rx_log_file);
        itsg5_caster_deinit();
    }
    else if(operation_mode==1) { /* Sending CAMs */

        itsg5_caster_init();

        // Load station setting from configuration file 
        ret_tx = load_station_config(&station_config);
        if (!IS_SUCCESS(ret_tx)) {
            printf("Failed to load station config, ret:%d!\n", ret_tx);
            return -1;
        }
        config_tx.ip = argv[1];
        config_tx.caster_id = 0;
        config_tx.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        // If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority
        // If the example is run on other platforms, it is optional to set tx and rx message threads name and priority
        itsg5_get_app_thread_config(&app_thread_config_tx);
        ret_tx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx.tx_thread_name, app_thread_config_tx.tx_thread_priority_low);

        tx_log_file = fopen(strcat(aux_path, "/tx.log"), "a");
        strcpy(aux_path, files_path);
        if(tx_log_file == NULL) printf("Error creating status file!");

        tx_counter = 0;
        cam_send(&config_tx, is_secured, security_protocol, evaluation_mode);

        fprintf(tx_log_file, "Transmitted %d messages\n", tx_counter);
        fclose(tx_log_file);
        itsg5_caster_deinit();
        
    } 
    else if(operation_mode==2) { /* Receiving and Sending CAMs*/

        itsg5_caster_init();

        config_tx.ip = argv[1];
        config_tx.caster_id = 0;
        config_tx.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        config_rx.ip = argv[1];
        config_rx.caster_id = 0;
        config_rx.caster_comm_mode = ITSG5_CASTER_MODE_RX;

        /* If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority */
        /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
        itsg5_get_app_thread_config(&app_thread_config_tx);
        ret_tx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx.tx_thread_name, app_thread_config_tx.tx_thread_priority_low);

        itsg5_get_app_thread_config(&app_thread_config_rx);
        ret_rx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, app_thread_config_rx.rx_thread_name, app_thread_config_rx.rx_thread_priority_low);

        // Creating log files
        rx_log_file = fopen(strcat(aux_path, "/rx.log"), "a");
        strcpy(aux_path, files_path);
        tx_log_file = fopen(strcat(aux_path, "/tx.log"), "a");
        strcpy(aux_path, files_path);
        
        if(rx_log_file == NULL) printf("Error creating status file!"); 
        if(rx_log_file == NULL) printf("Error creating rx file!");

        tx_counter = 0;
        rx_counter = 0;
        //Creating thread to receive messages
        pthread_t tid;

        pthread_create(&tid, NULL, cam_recv, &config_rx);

        cam_send(&config_tx, is_secured, security_protocol, evaluation_mode);

        fprintf(tx_log_file, "Transmitted %d messages\n", tx_counter);
        fprintf(rx_log_file, "Received %d messages\n", rx_counter);

        fclose(tx_log_file);
        fclose(rx_log_file);
        
        itsg5_caster_deinit();

    } 
    else if(operation_mode==3) { /* Latency - Echo CAMs */

        itsg5_caster_init();

        // Load station setting from configuration file 
        ret_tx = load_station_config(&station_config);
        if (!IS_SUCCESS(ret_tx)) {
            printf("Failed to load station config, ret:%d!\n", ret_tx);
            return -1;
        }

        config_tx.ip = argv[1];
        config_tx.caster_id = 0;
        config_tx.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        config_rx.ip = argv[1];
        config_rx.caster_id = 0;
        config_rx.caster_comm_mode = ITSG5_CASTER_MODE_RX;

        /* If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority */
        /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
        itsg5_get_app_thread_config(&app_thread_config_tx);
        ret_tx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx.tx_thread_name, app_thread_config_tx.tx_thread_priority_low);

        itsg5_get_app_thread_config(&app_thread_config_rx);
        ret_rx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, app_thread_config_rx.rx_thread_name, app_thread_config_rx.rx_thread_priority_low);

        // Creating log files
        rx_log_file = fopen(strcat(aux_path, "/rx.log"), "a");
        strcpy(aux_path, files_path);
        tx_log_file = fopen(strcat(aux_path, "/tx.log"), "a");
        strcpy(aux_path, files_path);


        if(rx_log_file == NULL) printf("Error creating rx file!");
        if(tx_log_file == NULL) printf("Error creating tx file!");

        tx_counter = 0;
        rx_counter = 0;

        cam_recv_lat(&config_tx, &config_rx);

        fprintf(tx_log_file, "Transmitted %d messages\n", tx_counter);
        fprintf(rx_log_file, "Received %d messages\n", rx_counter);

        fclose(tx_log_file);
        fclose(rx_log_file);
        itsg5_caster_deinit();

    } 
    else if(operation_mode==4) { /* Latency - Calculate latency on receiving */
        itsg5_caster_init();

        // Load station setting from configuration file 
        ret_tx = load_station_config(&station_config);
        if (!IS_SUCCESS(ret_tx)) {
            printf("Failed to load station config, ret:%d!\n", ret_tx);
            return -1;
        }

        config_tx.ip = argv[1];
        config_tx.caster_id = 0;
        config_tx.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        config_rx.ip = argv[1];
        config_rx.caster_id = 0;
        config_rx.caster_comm_mode = ITSG5_CASTER_MODE_RX;

        /* If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority */
        /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
        itsg5_get_app_thread_config(&app_thread_config_tx);
        ret_tx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx.tx_thread_name, app_thread_config_tx.tx_thread_priority_low);

        itsg5_get_app_thread_config(&app_thread_config_rx);
        ret_rx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, app_thread_config_rx.rx_thread_name, app_thread_config_rx.rx_thread_priority_low);

        // Creating log files
        rx_log_file = fopen(strcat(aux_path, "/rx.log"), "a");
        strcpy(aux_path, files_path);
        tx_log_file = fopen(strcat(aux_path, "/tx.log"), "a");
        strcpy(aux_path, files_path);
        latency = fopen(strcat(aux_path, "/latency_info.csv"), "a");
        strcpy(aux_path, files_path);
        latency2 = fopen(strcat(current_path, "/current.csv"), "w");

        if(rx_log_file  == NULL) printf("Error creating rx file!");
        if(tx_log_file  == NULL) printf("Error creating tx file!");
        if(latency      == NULL) printf("Error creating latency file!");
        if(latency2     == NULL) printf("Error creating latency file!");

        tx_counter = 0;
        rx_counter = 0;
        rx_lat_counter = 0;
        avgLatency = 0.0;
        pthread_t tid;

        pthread_create(&tid, NULL, cam_send_lat, &config_tx);
        cam_recv_lat(&config_tx, &config_rx);

        fprintf(tx_log_file, "Transmitted %d messages\n", tx_counter);
        fprintf(rx_log_file, "Received %d messages\n", rx_counter);

        fclose(tx_log_file);
        fclose(rx_log_file);
        fclose(latency);
        fclose(latency2);
        itsg5_caster_deinit();
    }
    else if(operation_mode==5) { /* Latency - Both methods*/
        itsg5_caster_init();

        // Load station setting from configuration file 
        ret_tx = load_station_config(&station_config);
        if (!IS_SUCCESS(ret_tx)) {
            printf("Failed to load station config, ret:%d!\n", ret_tx);
            return -1;
        }

        config_tx.ip = argv[1];
        config_tx.caster_id = 0;
        config_tx.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        config_tx_echo.ip = argv[1];
        config_tx_echo.caster_id = 0;
        config_tx_echo.caster_comm_mode = ITSG5_CASTER_MODE_TX;

        config_rx.ip = argv[1];
        config_rx.caster_id = 0;
        config_rx.caster_comm_mode = ITSG5_CASTER_MODE_RX;

        /* If the example is run in Unex device, please using the below functions to set tx and rx message threads name and priority */
        /* If the example is run on other platforms, it is optional to set tx and rx message threads name and priority */
        itsg5_get_app_thread_config(&app_thread_config_tx);
        ret_tx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx.tx_thread_name, app_thread_config_tx.tx_thread_priority_low);

        itsg5_get_app_thread_config(&app_thread_config_tx_echo);
        ret_tx_echo = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, app_thread_config_tx_echo.tx_thread_name, app_thread_config_tx_echo.tx_thread_priority_low);

        itsg5_get_app_thread_config(&app_thread_config_rx);
        ret_rx = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, app_thread_config_rx.rx_thread_name, app_thread_config_rx.rx_thread_priority_low);

        // Creating log files
        rx_log_file = fopen(strcat(aux_path, "/rx.log"), "a");
        strcpy(aux_path, files_path);
        tx_log_file = fopen(strcat(aux_path, "/tx.log"), "a");
        strcpy(aux_path, files_path);
        latency = fopen(strcat(aux_path, "/latency_info.csv"), "a");
        strcpy(aux_path, files_path);

        latency2 = fopen(strcat(current_path, "/current.csv"), "w");

        if(rx_log_file  == NULL) printf("Error creating rx file!");
        if(tx_log_file  == NULL) printf("Error creating tx file!");
        if(latency      == NULL) printf("Error creating latency file!");

        tx_counter = 0;
        rx_counter = 0;
        rx_lat_counter = 0;
        avgLatency = 0.0;
        pthread_t tid;

        pthread_create(&tid, NULL, cam_send_lat, &config_tx);
        cam_recv_lat(&config_tx_echo, &config_rx);

        fprintf(tx_log_file, "Transmitted %d messages\n", tx_counter);
        fprintf(rx_log_file, "Received %d messages\n", rx_counter);

        fclose(tx_log_file);
        fclose(rx_log_file);
        fclose(latency);
        fclose(latency2);
        itsg5_caster_deinit();
    } 
    else {
        printf("Operation Mode not recognized");
    }
    
    fclose(status_log_file);
    
    return 0;
}

static void cam_recv_lat(itsg5_caster_comm_config_t *tx_config, itsg5_caster_comm_config_t *rx_config)
{

    // Message Receiving variables

    itsg5_rx_info_t rx_info = {0};
    uint8_t rx_buf[ITSG5_CASTER_PKT_SIZE_MAX]; // Buffer for messages received
    size_t len;
    int ret;
    itsg5_caster_handler_t handler_rx = ITSG5_INVALID_CASTER_HANDLER;

    ret = itsg5_caster_create(&handler_rx, rx_config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return;
    }

    // Message Sending variables

    int ret2;
    poti_handler_t poti_handler = POTI_INVALID_CASTER_HANDLER;
    itsg5_caster_handler_t handler_tx = ITSG5_INVALID_CASTER_HANDLER;

    ret2 = itsg5_caster_create(&handler_tx, tx_config);
    if (!IS_SUCCESS(ret2)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret2, ERROR_MSG(ret2));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return;
    }

    poti_comm_config_t poti_config = {.ip = tx_config->ip};
    ret2 = poti_caster_create(&poti_handler, &poti_config);
    if (!IS_SUCCESS(ret2)) {
        printf("Fail to create POTI caster, ret:%d!\n", ret2);
        return;
    }  

    // Loop that receives messages and echo them

    if(operation_mode == 4 || operation_mode == 5){
        fprintf(latency, "ID, Round-Trip Time, Latency, Latency (Without Outliers)\n");
        fprintf(latency2, "ID, Round-Trip Time, Latency, Latency (Without Outliers)\n");
    }
    

    while (app_running) {

        // Receive a message
        ret = itsg5_caster_rx(handler_rx, &rx_info, rx_buf, sizeof(rx_buf), &len);

        if (IS_SUCCESS(ret)) {

            long long actTime = getTimestamp();
            struct process_message_struct *args = malloc(sizeof(struct process_message_struct));
            
            args->len = len;
            args->rx_info = rx_info;
            //args->rx_buf = rx_buf;
            memcpy(args->rx_buf, rx_buf, ITSG5_CASTER_PKT_SIZE_MAX);
            args->actTime = actTime;
            args->handler_tx = handler_tx;

            pthread_t threadID;
            pthread_create(&threadID, NULL, processMessage, args);
        }
        else {
            printf("Failed to receive data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
        }
        fflush(stdout);
        fflush(rx_log_file);
        fflush(latency);
        fflush(latency2);
    }
    itsg5_caster_release(handler_rx);
    itsg5_caster_release(handler_tx);
}

void * processMessage(void *_args)
{
    struct process_message_struct *args = (struct process_message_struct *) _args;

    // caster handler
    itsg5_caster_handler_t tx_handle = ITSG5_INVALID_CASTER_HANDLER;

    // communication config 
    // the port number must be defined in the JSON config 
    // comm_config_t txcfg = {.ip = "127.0.0.1", .caster_id = 0, .caster_comm_mode = ITSG5_CASTER_MODE_TX};
    
    itsg5_caster_comm_config_t txcfg;

    txcfg.ip = "127.0.0.1";
    txcfg.caster_id = 0;
    txcfg.caster_comm_mode = ITSG5_CASTER_MODE_TX;


    // create ITSG5 caster 
    int ret43 = itsg5_caster_create(&tx_handle, &txcfg);

    int ret = itsg5_caster_tx(tx_handle, &args->rx_info, args->rx_buf, (size_t)args->len);

    /*printf("Received %zu bytes!\n", args->len);
    fprintf(rx_log_file, "Received %zu bytes!\n", args->len);

    // Display ITS-G5 RX information
    dump_rx_info(&args->rx_info);

    // Check message StationID
    int index = check_source(args->rx_buf, (int)args->len, &args->rx_info);
    
    if(index != -1){
        
        // The messages was sent by this OBU
        printf("\nSame StationID\n");

        // Calculate latency

        if(operation_mode == 4 || operation_mode == 5){
            
            long long timeStamp = timestamps[index];
            long long rtt = args->actTime - timeStamp;
            float lat = rtt/2.0;

            if(avgLatency == 0.0){
                avgLatency = lat;
            } else {
                avgLatency = avgLatency + ((lat - avgLatency) / rx_lat_counter);
            }

            printf("Index: %d | Actual: %lld | Initial: %lld \n", index, args->actTime, timeStamp);
            printf("Index: %d | Latency: %.2f | Average Latency: %.2f", index, lat, avgLatency);
            fprintf(latency, "%d, %lld, %.2f, %.2f\n", index, rtt, lat, lat);
            fprintf(latency2, "%d, %lld, %.2f, %.2f\n", index, rtt, lat, lat);

            rx_lat_counter++;
        }
    } else {
        // The messages was sent by other OBU

        if(operation_mode == 3 || operation_mode == 5){
            printf("Before ITSG5_Caster_tx\n");

            // caster handler
            itsg5_caster_handler_t tx_handle = ITSG5_INVALID_CASTER_HANDLER;

            // communication config 
            // the port number must be defined in the JSON config 
            // comm_config_t txcfg = {.ip = "127.0.0.1", .caster_id = 0, .caster_comm_mode = ITSG5_CASTER_MODE_TX};
            
            itsg5_caster_comm_config_t txcfg;

            txcfg.ip = "127.0.0.1";
            txcfg.caster_id = 0;
            txcfg.caster_comm_mode = ITSG5_CASTER_MODE_TX;


            // create ITSG5 caster 
            int ret43 = itsg5_caster_create(&tx_handle, &txcfg);


            int ret = itsg5_caster_tx(tx_handle, &args->rx_info, args->rx_buf, (size_t)args->len);
            printf("\nDifferent Station ID, sent back\n");
        } else {
            printf("Different Station ID\n");
        }
    }*/

    rx_counter++;
    free(args);
    pthread_exit(NULL);
}

static void cam_send_lat(itsg5_caster_comm_config_t *config)
{
    
    uint8_t *tx_buf = NULL;
    int tx_buf_len = 0;
    int ret;
    poti_fix_data_t fix_data = {0};
    poti_gnss_info_t gnss_info = {0};
    itsg5_tx_info_t tx_info = {0}; /* According to C99, all tx_info members will be set to 0 */
    itsg5_caster_handler_t handler = ITSG5_INVALID_CASTER_HANDLER;
    poti_handler_t poti_handler = POTI_INVALID_CASTER_HANDLER;

    ret = itsg5_caster_create(&handler, config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return;
    }

    poti_comm_config_t poti_config = {.ip = config->ip};
    ret = poti_caster_create(&poti_handler, &poti_config);
    if (!IS_SUCCESS(ret)) {
        printf("Fail to create POTI caster, ret:%d!\n", ret);
        return;
    }

    while (app_running) {
        // EVAL COMMENT
        //printf("-----------------------\n");

        /* Get GNSS fix data from POTI caster service */
        ret = poti_caster_fix_data_rx(poti_handler, &fix_data);

        if (ret != 0) {
            printf("Fail to receive GNSS fix data from POTI caster service, %d\n", ret);
            /* Waiting for POTI caster service startup */
            usleep(1000000);
            continue;
        }
        else if (fix_data.mode < POTI_FIX_MODE_2D) {
            printf("GNSS not fix, fix mode: %d\n", fix_data.mode);

            /* Optional, APIs for users to get more GNSS information */
            ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
            if (IS_SUCCESS(ret)) {
                printf("GNSS antenna status:%d, time sync status: %d\n", gnss_info.antenna_status, gnss_info.time_sync_status);
            }

            /* The interval of get GNSS fix data is depend on GNSS data rate */
            usleep(100000);
            continue;
        }

        /* Optional, NAN value check for GNSS data */
        if ((isnan(fix_data.latitude) == 1) || (isnan(fix_data.longitude) == 1) || (isnan(fix_data.altitude) == 1) || (isnan(fix_data.horizontal_speed) == 1) || (isnan(fix_data.course_over_ground) == 1)) {
            printf("GNSS fix data has NAN value, latitude: %f, longitude : %f, altitude : %f, horizontal speed : %f, course over ground : %f\n", fix_data.latitude, fix_data.longitude, fix_data.altitude, fix_data.horizontal_speed, fix_data.course_over_ground);
            continue;
        }

        ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
        if (IS_SUCCESS(ret)) {
            printf("Access layer time sync state: %d , unsync times: %d\n", gnss_info.acl_time_sync_state, gnss_info.acl_unsync_times);
        }

        cam_encode(&tx_buf, &tx_buf_len, &fix_data, 0, 0);
        if (tx_buf != NULL) {
            /*//EVAL COMMENT:
            printf("CAM encoded data:\n");
            dump_mem(tx_buf, tx_buf_len);*/
            set_tx_info(&tx_info, is_secured_lat);

            ret = itsg5_caster_tx(handler, &tx_info, tx_buf, (size_t)tx_buf_len);
            if (IS_SUCCESS(ret)) {
                printf("Transmitted %zu bytes!\n", tx_buf_len);
                fprintf(tx_log_file, "\tTransmitted %zu bytes!\n", tx_buf_len);
            }
            else {
                printf("Failed to transmit data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
            }
            
            long long tmstmp = getTimestamp();
            timestamps[tx_counter%1000] = tmstmp;
            printf("Index: %d | Timestamp: %lld\n", tx_counter%1000, tmstmp);
            
            itsmsg_buf_free(tx_buf);
        }

        tx_counter++;

        fflush(stdout);
        fflush(tx_log_file);
        

        /* This time interval is an example only, the message generation interval please refer to ETSI EN 302 637-2 */
        sleep(1);
    }

    itsg5_caster_release(handler);
    poti_caster_release(poti_handler);
}

static int check_source(uint8_t *p_rx_payload, int rx_payload_len, itsg5_rx_info_t *p_itsg5_rx_info)
{
    ITSMsgCodecErr err;
    ItsPduHeader *p_rx_decode_hdr = NULL;
    CAM *p_rx_decode_cam = NULL;
    CAM_V2 *p_rx_decode_cam_v2 = NULL;
    int result;

    /* Allocate a buffer for restoring the decode error information if needed. */
    err.msg_size = 256;
    err.msg = calloc(1, err.msg_size);

    if (err.msg == NULL) {
        printf("Cannot allocate memory for error message buffer.\n");
        return -1;
    }

    /* Determine and decode the content in RX payload. */
    result = itsmsg_decode(&p_rx_decode_hdr, p_rx_payload, rx_payload_len, &err);

    /* Check whether this is a ITS message. */
    if (result > 0 && p_rx_decode_hdr != NULL) {
        /* Check whether this is a ITS CAM message. */
        if (p_rx_decode_hdr->messageID == CAM_Id) {
            /* Display ITS message version. */
            printf("ITS msg protocol version: v%d\n", p_rx_decode_hdr->protocolVersion);

            /* Mapping data format base on protocol version. */
            switch (p_rx_decode_hdr->protocolVersion) {
                case 1:
                    /* Convert to version 1 CAM data format. */
                    p_rx_decode_cam = (CAM *)p_rx_decode_hdr;

                    /* Extract other message element, the example only extract contents of newest version Unex supported */
                    printf("[ Received CAM from station %u ]\n", p_rx_decode_cam->header.stationID);
                    break;
                case 2:
                    /* Convert to version 2 CAM data format. */
                    p_rx_decode_cam_v2 = (CAM_V2 *)p_rx_decode_hdr;

                    /* Check CAM msg permission */
                    if (p_itsg5_rx_info->security.status == ITSG5_DECAP_VERIFIED_PKT) {
                        result = cam_check_msg_permission(p_rx_decode_cam_v2, p_itsg5_rx_info->security.ssp, p_itsg5_rx_info->security.ssp_len);
                        printf("Check msg permissions: ");
                        if (IS_SUCCESS(result)) {
                            printf("trustworthy\n");
                        }
                        else {
                            printf("untrustworthy\n");
                        }
                    }
                    
                    if(p_rx_decode_cam_v2->header.stationID == station_config.stationID){
                        int genDeltaTime = p_rx_decode_cam_v2->cam.generationDeltaTime;
                        printf("\nMessage echoed: %d\n", genDeltaTime);
                        return genDeltaTime;
                    }

                    /* Display the decoded CAM content. */
                    // EVAL COMMENT:
                    /*printf("[ Received CAM from station %u ]\n", p_rx_decode_cam_v2->header.stationID);
                    printf("\tStation type: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.stationType);
                    printf("\tgenerationDeltaTime: %d\n", p_rx_decode_cam_v2->cam.generationDeltaTime);
                    printf("\tLatitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.latitude);
                    printf("\tLongitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.longitude);
                    printf("\tAltitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue);*/
                    
                    fprintf(rx_log_file, "[%s]:\n", getCurrentTime());
                    fprintf(rx_log_file,"[ Received CAM from station %u ]\n", p_rx_decode_cam_v2->header.stationID);
                    fprintf(rx_log_file,"\tStation type: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.stationType);
                    fprintf(rx_log_file,"\tgenerationDeltaTime: %d\n", p_rx_decode_cam_v2->cam.generationDeltaTime);
                    fprintf(rx_log_file,"\tLatitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.latitude);
                    fprintf(rx_log_file,"\tLongitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.longitude);
                    fprintf(rx_log_file,"\tAltitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue);

                    break;
                default:
                    printf("Received unsupported CAM protocol version: %d\n", p_rx_decode_hdr->protocolVersion);
                    break;
            }
        }
        else {
            printf("Received unrecognized ITS message type: %d\n", p_rx_decode_hdr->messageID);
        }

        /* Release the decode message buffer. */
        itsmsg_free(p_rx_decode_hdr);
    }
    else {
        printf("Unable to decode RX packet: %s\n", err.msg);
    }

    /* Release the allocated error message buffer. */
    free(err.msg);

    return -1;
}

static void cam_encode(uint8_t **tx_buf, int *tx_buf_len, poti_fix_data_t *p_fix_data, int security_protocol, int evaluation_mode)
{

    // Evaluation zone: ---------------- ---------------- ---------------- ----------------
    struct timespec start_time, end_time;
    // ---------------- ---------------- ---------------- ---------------- ----------------

    ITSMsgCodecErr err;
    CAM_V2 cam_tx_encode_fmt;
    int ret;

    /* Load station setting from configuration file */
    ret = load_station_config(&station_config);
    if (-1 == ret) {
        printf("Using fixed CAM data.\n");
    }
    /* Make sure we reset the data structure at least once. */
    memset((void *)&cam_tx_encode_fmt, 0, sizeof(cam_tx_encode_fmt));

    /* For the present document, the value of the DE protocolVersion shall be set to 1.  */
    cam_tx_encode_fmt.header.protocolVersion = CAM_PROTOCOL_VERSION;
    cam_tx_encode_fmt.header.messageID = CAM_Id;
    if (0 == ret) {
        /* Set stationID form station config file */
        cam_tx_encode_fmt.header.stationID = station_config.stationID;
    }
    else {
        /* Set fixed stationID*/
        cam_tx_encode_fmt.header.stationID = CAM_STATION_ID_DEF;
    }


    /*
     * Time corresponding to the time of the reference position in the CAM, considered
     * as time of the CAM generation.
     * The value of the DE shall be wrapped to 65 536. This value shall be set as the
     * remainder of the corresponding value of TimestampIts divided by 65 536 as below:
     *      generationDeltaTime = TimestampIts mod 65 536
     * TimestampIts represents an integer value in milliseconds since 2004-01-01T00:00:00:000Z
     * as defined in ETSI TS 102 894-2 [2].
     */
    cam_tx_encode_fmt.cam.generationDeltaTime = (int32_t) tx_counter % 1000;
    // cam_tx_encode_fmt.cam.generationDeltaTime = (int32_t)fmod(p_fix_data->time.tai_since_2004 * 1000.0, 65536); /* TAI milliseconds since 2004-01-01 00:00:00.000 UTC. */

    /*
	 * Position and position accuracy measured at the reference point of the originating
	 * ITS-S. The measurement time shall correspond to generationDeltaTime.
	 * If the station type of the originating ITS-S is set to one out of the values 3 to 11
	 * the reference point shall be the ground position of the centre of the front side of
	 * the bounding box of the vehicle.
	 * The positionConfidenceEllipse provides the accuracy of the measured position
	 * with the 95 % confidence level. Otherwise, the positionConfidenceEllipse shall be
	 * set to unavailable.
	 * If semiMajorOrientation is set to 0 degree North, then the semiMajorConfidence
	 * corresponds to the position accuracy in the North/South direction, while the
	 * semiMinorConfidence corresponds to the position accuracy in the East/West
	 * direction. This definition implies that the semiMajorConfidence might be smaller
	 * than the semiMinorConfidence.
	 */
    cam_tx_encode_fmt.cam.camParameters.basicContainer.stationType = CAM_STATION_TYPE_DEF;
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.latitude = (int32_t)(p_fix_data->latitude * 10000000.0); /* Convert to 1/10 micro degree. */
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.longitude = (int32_t)(p_fix_data->longitude * 10000000.0); /* Convert to 1/10 micro degree. */
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = (int32_t)(p_fix_data->altitude * 100.0);
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorConfidence = cam_set_semi_axis_length(p_fix_data->err_smajor_axis); /* Convert to centimetre. */
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMinorConfidence = cam_set_semi_axis_length(p_fix_data->err_sminor_axis); /* Convert to centimetre. */
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.positionConfidenceEllipse.semiMajorOrientation = cam_set_heading_value(p_fix_data->err_smajor_orientation);
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.altitude.altitudeConfidence = cam_set_altitude_confidence(p_fix_data->err_altitude);
    cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue = (int32_t)(p_fix_data->altitude * 100.0);
    /*
     * The mandatory high frequency container of CAM.
     */

    /* Heading. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.heading.headingValue = (int32_t)(p_fix_data->course_over_ground * 10.0); /* Convert to 0.1 degree from North. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.heading.headingConfidence = cam_set_heading_confidence(p_fix_data->err_course_over_ground); /* Convert to 1 ~ 127 enumeration. */

    /* Speed, 0.01 m/s */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.speed.speedValue = (int16_t)(p_fix_data->horizontal_speed * 100.0); /* Convert to 0.01 metre per second. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.speed.speedConfidence = cam_set_speed_confidence(p_fix_data->err_horizontal_speed);

    /* Direction. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.driveDirection = CAM_SENSOR_GET_DRIVE_DIRECTION();

    /* Vehicle length, 0.1 metre  */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthValue = CAM_SENSOR_GET_VEHICLE_LENGTH_VALUE();
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.vehicleLength.vehicleLengthConfidenceIndication = CAM_SENSOR_GET_VEHICLE_LENGTH_CONF();

    /* Vehicle width, 0.1 metre */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.vehicleWidth = CAM_SENSOR_GET_VEGICLE_WIDTH_VALUE();

    /* Longitudinal acceleration, 0.1 m/s^2 */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationValue = CAM_SENSOR_GET_LONG_ACCEL_VALUE();
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.longitudinalAcceleration.longitudinalAccelerationConfidence = CAM_SENSOR_GET_LONG_ACCEL_CONF();

    /*
     * Curvature value, 1 over 30000 meters, (-30000 .. 30001)
     * The confidence value shall be set to:
     *      0 if the accuracy is less than or equal to 0,00002 m-1
     *      1 if the accuracy is less than or equal to 0,0001 m-1
     *      2 if the accuracy is less than or equal to 0,0005 m-1
     *      3 if the accuracy is less than or equal to 0,002 m-1
     *      4 if the accuracy is less than or equal to 0,01 m-1
     *      5 if the accuracy is less than or equal to 0,1 m-1
     *      6 if the accuracy is out of range, i.e. greater than 0,1 m-1
     *      7 if the information is not available
     */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.curvature.curvatureValue = CAM_SENSOR_GET_CURVATURE_VALUE();
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.curvature.curvatureConfidence = CAM_SENSOR_GET_CURVATURE_CONF();
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.curvatureCalculationMode = CAM_SENSOR_GET_CURVATURE_CONF_CAL_MODE();

    /* YAW rate, 0,01 degree per second. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.yawRate.yawRateValue = CAM_SENSOR_GET_YAW_RATE_VALUE();
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.yawRate.yawRateConfidence = CAM_SENSOR_GET_YAW_RATE_CONF();

    /* Optional fields, disable all by default. */
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.accelerationControl_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.accelerationControl =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.lanePosition_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.lanePosition =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.steeringWheelAngle_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.steeringWheelAngle =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.lateralAcceleration_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.lateralAcceleration =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.verticalAcceleration_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.verticalAcceleration =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.performanceClass_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.performanceClass =
    cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.cenDsrcTollingZone_option = FALSE;
    //cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.cenDsrcTollingZone =


    /* 
    * ------------------------------------------------------------------------------------------------------------------------------------------------
    * Encapsulation code START -----------------------------------------------------------------------------------------------------------------------
    * ------------------------------------------------------------------------------------------------------------------------------------------------
    */

    if(security_protocol == 1) {

        // ------------------------------------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------------------------------------ DLAPP ENCAPSULATION START
        // ------------------------------------------------------------------------------------------------------------------------

        /******************/
        /* Hardcoded data */
        /******************/

        // 20 bytes each pseudo id
        unsigned char pseudo_ids[5][20] = {
            {240, 9, 204, 205, 214, 160, 104, 101, 7, 17, 165, 42, 162, 114, 144, 175, 199, 162, 106, 216},
            {187, 56, 198, 22, 44, 167, 248, 205, 204, 38, 186, 25, 108, 255, 31, 105, 78, 199, 130, 60},
            {143, 15, 144, 71, 42, 90, 229, 34, 44, 106, 170, 164, 72, 117, 173, 114, 98, 255, 218, 84},
            {184, 165, 136, 38, 140, 22, 115, 116, 201, 3, 152, 138, 80, 63, 132, 5, 189, 245, 243, 90},
            {155, 84, 205, 254, 51, 156, 69, 170, 190, 90, 159, 12, 206, 206, 206, 164, 6, 47, 47, 134}
        };

        // 32 bytes each hash chain key
        unsigned char hash_chain_keys[5][32] = {
            {240, 9, 204, 205, 214, 160, 104, 101, 7, 17, 165, 42, 184, 56, 112, 76, 103, 115, 237, 223, 31, 137, 82, 113, 162, 114, 144, 175, 199, 162, 106, 216},
            {187, 56, 198, 22, 44, 167, 248, 205, 171, 174, 251, 201, 221, 205, 237, 123, 186, 151, 123, 199, 204, 38, 186, 25, 108, 255, 31, 105, 78, 199, 130, 60},
            {143, 15, 144, 71, 42, 90, 229, 34, 228, 80, 37, 227, 153, 54, 218, 239, 70, 48, 231, 172, 44, 106, 170, 164, 72, 117, 173, 114, 98, 255, 218, 84},
            {184, 165, 136, 38, 140, 22, 194, 211, 128, 201, 99, 36, 10, 215, 122, 191, 135, 195, 115, 116, 201, 3, 152, 138, 80, 63, 132, 5, 189, 245, 243, 90},
            {155, 84, 205, 254, 51, 156, 175, 155, 41, 220, 171, 121, 83, 82, 215, 202, 144, 56, 69, 170, 190, 90, 159, 12, 206, 206, 206, 164, 6, 47, 47, 134}
        };


        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 1) {
            // Get initial timestamp in milliseconds precision
            clock_gettime(CLOCK_MONOTONIC, &start_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------

        // Seed the random number generator
        srand(time(NULL));


        /*****************/
        /*** Pseudo Id ***/
        /*****************/

        // Get a random index for the outer array
        int random_pseudo_id_index = rand() % 5;
        unsigned char* random_pseudo_id = pseudo_ids[random_pseudo_id_index];

        /*****************/
        /****** Key ******/
        /*****************/

        // Get a hash chain for the outer array (0 to 4)
        int random_hash_chain_index = rand() % 5;

        unsigned char key_bytes[4];

        // Convert the int to bytes using bitwise operations
        key_bytes[0] = (random_hash_chain_index >> 24) & 0xFF; // Extract the least significant byte
        key_bytes[1] = (random_hash_chain_index >> 16) & 0xFF; // Extract the second least significant byte
        key_bytes[2] = (random_hash_chain_index >> 8) & 0xFF; // Extract the third least significant byte
        key_bytes[3] = (random_hash_chain_index >> 0) & 0xFF; // Extract the most significant byte

        /*****************/
        /*** Timestamp ***/
        /*****************/

        int32_t current_ts = (int32_t)time(NULL);

        // Convert the int32_t to bytes
        unsigned char ts_bytes[4];
        ts_bytes[0] = (current_ts >> 24) & 0xFF; // Most significant byte
        ts_bytes[1] = (current_ts >> 16) & 0xFF;
        ts_bytes[2] = (current_ts >> 8) & 0xFF;
        ts_bytes[3] = current_ts & 0xFF; // Least significant byte

        /*****************/
        /****** MAC ******/
        /*****************/

        /***** First, we shall get the original message, so we can use it hte MAC operation *****/

        ITSMsgCodecErr err_temp;
        uint8_t *its_message_bytes  = NULL;
        uint8_t **temp_buf = &its_message_bytes ;
        int its_message_bytes_len = 0;

        /* Allocate a buffer for restoring the decode error information if needed. */
        err_temp.msg_size = 512;
        err_temp.msg = calloc(1, err_temp.msg_size);
        
        //ItsPduHeader *header = (ItsPduHeader *)&cam_tx_encode_fmt;

        if (err_temp.msg != NULL) {

            /* Encode the CAM message. */
            its_message_bytes_len = itsmsg_encode(temp_buf, (ItsPduHeader *)&cam_tx_encode_fmt, &err_temp);

            if (its_message_bytes_len <= 0) {
                printf("itsmsg_encode error v2: %s\n", err_temp.msg);
            }

            free(err_temp.msg);
        }
        else {
            printf("Cannot allocate memory for error message buffer.\n");
        }

        // Concatenate the byte arrays to calculate its mac

        // Calculate the total length of the resulting byte array
        size_t total_length = sizeof(pseudo_ids[random_pseudo_id_index]) + its_message_bytes_len + sizeof(ts_bytes);

        // Create the resulting byte array
        unsigned char *msg_to_hmac = (unsigned char*)malloc(total_length);

        // Concatenate the byte arrays
        size_t index = 0;
        
        // Copy bytes from pid_bytes to the result array
        for (size_t i = 0; i < sizeof(pseudo_ids[random_pseudo_id_index]); i++) {
            msg_to_hmac[index++] = random_pseudo_id[i];
        }
        
        // Copy bytes from its_message_bytes to the result array
        for (size_t i = 0; i < its_message_bytes_len; i++) {
            msg_to_hmac[index++] = its_message_bytes[i];
        }
        
        // Copy bytes from ts_bytes to the result array
        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
            msg_to_hmac[index++] = ts_bytes[i];
        }

        unsigned char hash[32]; // Output hash will be stored here

        size_t hashlen = hmac_sha256(hash_chain_keys[random_hash_chain_index], sizeof(hash_chain_keys[random_hash_chain_index]), msg_to_hmac, total_length, hash, sizeof(hash));


        // Now insert the security bytes inside the CAM: 20 bytes (pseudo id) + 12 bytes (mac) + 4 bytes (pseudo id num) + 4 bytes (timestamp)

        // Create the resulting byte array
        unsigned char *security_bytes = (unsigned char*)malloc(40);

        // Concatenate the byte arrays
        size_t index_to_concatenate = 0;
        
      
        // Copy bytes from pid_bytes to the sec bytes array
        for (size_t i = 0; i < sizeof(pseudo_ids[random_pseudo_id_index]); i++) {
            security_bytes[index_to_concatenate++] = random_pseudo_id[i];
        }
    
        // Copy bytes from hmac result to the sec bytes array
        for (size_t i = 20; i < hashlen; i++) {
            security_bytes[index_to_concatenate++] = hash[i];
        }
        
        // Copy bytes from random key index to the sec bytes array
        for (size_t i = 0; i < sizeof(key_bytes); i++) {
            security_bytes[index_to_concatenate++] = key_bytes[i];
        }
        
        // Copy bytes from ts_bytes to the sec bytes array
        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
            security_bytes[index_to_concatenate++] = ts_bytes[i];
            //printf("%d ", (signed char) security_bytes[index_to_concatenate -1]);
        }

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 1) {
            // Get final timestamp in milliseconds precision
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            
            // Calculate the time difference with more precision
            double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
            
            printf("%.9lf\n", elapsed_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------

        

        // --------------------------------- Modify CAM message : Start | GOAL: to have a lowFrequencyContainer and then include the path points including the security fields

        cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer_option = true;
        if (0 == ret) {
            /* Set vehicleRole form station config file */
            cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.vehicleRole = station_config.role;
        }
        else {
            /* Set fixed vehicleRole*/
            cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.vehicleRole = VehicleRole_emergency;
        }

        /* alloc and set the bit of bitstring */
        asn1_bstr_alloc_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_MAX_BITS_ITS, ExteriorLights_highBeamHeadlightsOn_ITS);

        /* Set exteriorLights form station config file */
        if (station_config.leftTurnSignalOn) {
            asn1_bstr_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_leftTurnSignalOn_ITS);
        }
        if (station_config.rightTurnSignalOn) {
            asn1_bstr_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_rightTurnSignalOn_ITS);
        }

        PathHistory_ITS pathHistory;
        PathPoint pathPoints[10];

        for (int i = 0; i < 10; i++) {
            // Extract 4 bytes and assign them to deltaLatitude and deltaLongitude
            pathPoints[i].pathPosition.deltaLatitude = (int)((security_bytes[i * 4] << 8) | security_bytes[i * 4 + 1]);
            pathPoints[i].pathPosition.deltaLongitude = (int)((security_bytes[i * 4 + 2] << 8) | security_bytes[i * 4 + 3]);
            pathPoints[i].pathPosition.deltaAltitude = 0;
            pathPoints[i].pathDeltaTime_option = FALSE;
        }

        // Set PathHistory_ITS tab and count
        pathHistory.tab = pathPoints;
        pathHistory.count = 10; // Number of path points

        // Assign the PathHistory_ITS to BasicVehicleContainerLowFrequency
        cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory = pathHistory;

        // --------------------------------- Modify CAM message : END

    } else if (security_protocol == 2) {
        
        // ------------------------------------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------------------------------------ MFSPV ENCAPSULATION START
        // ------------------------------------------------------------------------------------------------------------------------

        /******************/
        /* Hardcoded data */
        /******************/
        unsigned char regional_key[32] = {240, 9, 204, 205, 214, 160, 104, 101, 7, 17, 165, 42, 184, 56, 112, 76, 103, 115, 237, 223, 31, 137, 82, 113, 162, 114, 144, 175, 199, 162, 106, 216};
        
        unsigned char api_new[32]={17, 19, 9, 120, 7, 12, 5, 80, 7, 19, 7, 19, 17, 19, 9, 120, 7, 12, 5, 80, 7, 19, 17, 19, 9, 120, 7, 12, 5, 80, 7, 19};
        unsigned char v_sk[32]={11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 19, 20, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
        unsigned char id_v[32]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        unsigned char k_mbr[32]={11, 19, 9, 120, 37, 12, 5, 80, 7, 19, 7, 19, 11, 19, 9, 120, 37, 12, 5, 80, 7, 19, 11, 19, 9, 120, 37, 12, 5, 80, 7, 19};

        size_t static_component_total_length = sizeof(api_new) + sizeof(v_sk) + sizeof(id_v) + sizeof(k_mbr);

        // Create the resulting byte array
        unsigned char *static_component = (unsigned char*)malloc(static_component_total_length);

        // Concatenate the byte arrays
        size_t index_sc = 0;
        
        // Copy bytes from api_new to the result array
        for (size_t i = 0; i < sizeof(api_new); i++) {
            static_component[index_sc++] = api_new[i];
        }

        // Copy bytes from v_sk to the result array
        for (size_t i = 0; i < sizeof(v_sk); i++) {
            static_component[index_sc++] = v_sk[i];
        }

        // Copy bytes from id_v to the result array
        for (size_t i = 0; i < sizeof(id_v); i++) {
            static_component[index_sc++] = id_v[i];
        }

        // Copy bytes from k_mbr to the result array
        for (size_t i = 0; i < sizeof(k_mbr); i++) {
            static_component[index_sc++] = k_mbr[i];
        }

        unsigned char hash_pid_static_component[SHA256_DIGEST_LENGTH];
        SHA256(static_component, static_component_total_length, hash_pid_static_component);

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 1) {
            // Get initial timestamp in milliseconds precision
            clock_gettime(CLOCK_MONOTONIC, &start_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------


        /*****************/
        /*** Timestamp ***/
        /*****************/

        // 4bytes
        int32_t current_ts = (int32_t)time(NULL);

        // Convert the int32_t to bytes
        unsigned char ts_bytes[4];
        ts_bytes[0] = (current_ts >> 24) & 0xFF; // Most significant byte
        ts_bytes[1] = (current_ts >> 16) & 0xFF;
        ts_bytes[2] = (current_ts >> 8) & 0xFF;
        ts_bytes[3] = current_ts & 0xFF; // Least significant byte

        /*printf("Timestamp: ");
        for (int i = 0; i < 4; i++) {
            printf("%d ", (signed char) ts_bytes[i]);
        }
        printf("\n");*/


        /****************/
        /**** PidGen ****/
        /****************/
        size_t dynamic_component_total_length = sizeof(api_new) + sizeof(ts_bytes);

        // Create the resulting byte array
        unsigned char *dynamic_component = (unsigned char*)malloc(dynamic_component_total_length);

        // Concatenate the byte arrays
        size_t index_dc = 0;
        
        // Copy bytes from api_new to the result array
        for (size_t i = 0; i < sizeof(api_new); i++) {
            dynamic_component[index_dc++] = api_new[i];
        }

        // Copy bytes from ts_bytes to the result array
        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
            dynamic_component[index_dc++] = ts_bytes[i];
        }

        unsigned char hash_pid_dynamic_component[SHA256_DIGEST_LENGTH];
        SHA256(dynamic_component, dynamic_component_total_length, hash_pid_dynamic_component);

        // Perform xor
        
        unsigned char pseudo_id[20]; 

        for (size_t i = 12; i < SHA256_DIGEST_LENGTH; i++) {
            pseudo_id[i - 12] = dynamic_component[i] ^ static_component[i];
        }
        
        /*printf("Pseudo ID: {");
        for (int i = 0; i < sizeof(pseudo_id); i++) {
            printf("%d ", (signed char) pseudo_id[i]);
        }*/

        /*****************/
        /****** MAC ******/
        /*****************/

        /***** First, we shall get the original message, so we can use it hte MAC operation *****/

        ITSMsgCodecErr err_temp;
        uint8_t *its_message_bytes  = NULL;
        uint8_t **temp_buf = &its_message_bytes ;
        int its_message_bytes_len = 0;

        /* Allocate a buffer for restoring the decode error information if needed. */
        err_temp.msg_size = 512;
        err_temp.msg = calloc(1, err_temp.msg_size);
        
        //ItsPduHeader *header = (ItsPduHeader *)&cam_tx_encode_fmt;

        if (err_temp.msg != NULL) {

            /* Encode the CAM message. */
            its_message_bytes_len = itsmsg_encode(temp_buf, (ItsPduHeader *)&cam_tx_encode_fmt, &err_temp);

            if (its_message_bytes_len <= 0) {
                printf("itsmsg_encode error v2: %s\n", err_temp.msg);
            }

            //EVAL COMMENT:
            /*printf("\nORIGINAL CAM encoded data:");
            dump_mem(its_message_bytes, its_message_bytes_len); // Prints data in hexa

            printf("ORIGINAL CAM size: %d\n\n", its_message_bytes_len);*/

            free(err_temp.msg);
        }
        else {
            printf("Cannot allocate memory for error message buffer.\n");
        }

        // Concatenate the byte arrays to calculate its mac

        // Calculate the total length of the resulting byte array
        size_t total_length = sizeof(pseudo_id) + sizeof(regional_key) + its_message_bytes_len + sizeof(ts_bytes);
        
        // Create the resulting byte array
        unsigned char *msg_to_hmac = (unsigned char*)malloc(total_length);


        // Concatenate the byte arrays
        size_t index = 0;
        
        // Copy bytes from pid_bytes to the result array
        for (size_t i = 0; i < sizeof(pseudo_id); i++) {
            msg_to_hmac[index++] = pseudo_id[i];
        }

        // Copy bytes from regional_key to the result array
        for (size_t i = 0; i < sizeof(regional_key); i++) {
            msg_to_hmac[index++] = regional_key[i];
        }
        
        // Copy bytes from its_message_bytes to the result array
        for (size_t i = 0; i < its_message_bytes_len; i++) {
            msg_to_hmac[index++] = its_message_bytes[i];
        }
        
        // Copy bytes from ts_bytes to the result array
        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
            msg_to_hmac[index++] = ts_bytes[i];
        }

        // msg_to_hmac
        /*printf("\n\nmsg_to_hmac bytes all:\n");
        for (size_t i = 0; i < total_length; i++) {
            printf("%d ", (signed char) msg_to_hmac[i]);
        }*/

        // Calculate the SHA-256 hash
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(msg_to_hmac, total_length, hash);

        // Now insert the security bytes inside the CAM: 20 bytes (pseudo id) + 12 bytes (mac) + 4 bytes (pseudo id num) + 4 bytes (timestamp)

        // Create the resulting byte array
        unsigned char *security_bytes = (unsigned char*)malloc(44);

        // Concatenate the byte arrays
        size_t index_to_concatenate = 0;

        //printf("\nSec bytes:\n");
        
        // Timestamp copy (4 bytes)
        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
            security_bytes[index_to_concatenate++] = ts_bytes[i];
            //printf("%d ", (signed char) security_bytes[index_to_concatenate -1]);
        }
        //printf("\n");

        // MAC copy (20 bytes)
        // Copy the last 20 bytes from hmac result to the sec bytes array
        for (size_t i = 12; i < SHA256_DIGEST_LENGTH; i++) {
            security_bytes[index_to_concatenate++] = hash[i];
            //printf("%d ", (signed char) security_bytes[index_to_concatenate -1]);
        }
        // Copy bytes from ts_bytes to the sec bytes array

        // Pseudo id copy (20 bytes)
        // Copy bytes from pid_bytes to the sec bytes array
        for (size_t i = 0; i < sizeof(pseudo_id); i++) {
            security_bytes[index_to_concatenate++] = pseudo_id[i];
            //printf("%d ", (signed char) security_bytes[index_to_concatenate -1]);
        }
        //printf("\n");

        /*printf("\n\n\nSec bytes all:\n");
        for (size_t i = 0; i < 40; i++) {
            printf("%d ", (signed char) security_bytes[i]);
        }
        printf("\n\n");*/

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 1) {
            // Get final timestamp in milliseconds precision
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            
            // Calculate the time difference with more precision
            double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
            
            printf("%.9lf\n", elapsed_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------


        // --------------------------------- Modify CAM message : Start | GOAL: to have a lowFrequencyContainer and then include the path points including the security fields

        cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer_option = true;
        if (0 == ret) {
            /* Set vehicleRole form station config file */
            cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.vehicleRole = station_config.role;
        }
        else {
            /* Set fixed vehicleRole*/
            cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.vehicleRole = VehicleRole_emergency;
        }

        /* alloc and set the bit of bitstring */
        asn1_bstr_alloc_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_MAX_BITS_ITS, ExteriorLights_highBeamHeadlightsOn_ITS);

        /* Set exteriorLights form station config file */
        if (station_config.leftTurnSignalOn) {
            asn1_bstr_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_leftTurnSignalOn_ITS);
        }
        if (station_config.rightTurnSignalOn) {
            asn1_bstr_set_bit(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights), ExteriorLights_rightTurnSignalOn_ITS);
        }

        PathHistory_ITS pathHistory;
        PathPoint pathPoints[11];

        for (int i = 0; i < 11; i++) {
            // Extract 4 bytes and assign them to deltaLatitude and deltaLongitude
            pathPoints[i].pathPosition.deltaLatitude = (int)((security_bytes[i * 4] << 8) | security_bytes[i * 4 + 1]);
            pathPoints[i].pathPosition.deltaLongitude = (int)((security_bytes[i * 4 + 2] << 8) | security_bytes[i * 4 + 3]);
            //Old try to use signed int: pathPoints[i].pathPosition.deltaLatitude = (int)((int8_t)security_bytes[i * 4] << 8 | (int8_t)security_bytes[i * 4 + 1]);
            //pathPoints[i].pathPosition.deltaLongitude = (int)((int8_t)security_bytes[i * 4 + 2] << 8 | (int8_t)security_bytes[i * 4 + 3]);
            pathPoints[i].pathPosition.deltaAltitude = 0;
            pathPoints[i].pathDeltaTime_option = FALSE;
        }

        // Set PathHistory_ITS tab and count
        pathHistory.tab = pathPoints;
        pathHistory.count = 11; // Number of path points

        // Assign the PathHistory_ITS to BasicVehicleContainerLowFrequency
        cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory = pathHistory;

        // --------------------------------- Modify CAM message : END

    }

    /* 
    * ------------------------------------------------------------------------------------------------------------------------------------------------
    * Encapsulation code END -------------------------------------------------------------------------------------------------------------------------
    * ------------------------------------------------------------------------------------------------------------------------------------------------
    */

    /* Allocate a buffer for restoring the decode error information if needed. */
    err.msg_size = 512;
    err.msg = calloc(1, err.msg_size);

    if (err.msg != NULL) {

        /* Encode the CAM message. */
        *tx_buf_len = itsmsg_encode(tx_buf, (ItsPduHeader *)&cam_tx_encode_fmt, &err);

        if (*tx_buf_len <= 0) {
            printf("itsmsg_encode error: %s\n", err.msg);
        }

        
        const char *time = getCurrentTime();
        fprintf(tx_log_file, "[%s]:\n", time);
        fprintf(tx_log_file, "\tStationType: %d\n", cam_tx_encode_fmt.cam.camParameters.basicContainer.stationType);
        fprintf(tx_log_file, "\tLatitude: %d\n", cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.latitude);
        fprintf(tx_log_file, "\tLongitude: %d\n", cam_tx_encode_fmt.cam.camParameters.basicContainer.referencePosition.longitude);
        fprintf(tx_log_file, "\tSpeed: %d\n", cam_tx_encode_fmt.cam.camParameters.highFrequencyContainer.u.basicVehicleContainerHighFrequency.speed.speedValue);

        /* Release the allocated error message buffer. */
        free(err.msg);
    }
    else {
        printf("Cannot allocate memory for error message buffer.\n");
    }

    if (cam_tx_encode_fmt.cam.camParameters.basicContainer.stationType == ITS_STATION_SPECIAL_VEHICLE) {
        /* free the memory for encoding */
        asn1_bstr_free(&(cam_tx_encode_fmt.cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.exteriorLights));
        asn1_bstr_free(&(cam_tx_encode_fmt.cam.camParameters.specialVehicleContainer.u.emergencyContainer.lightBarSirenInUse));
        asn1_bstr_free(&(cam_tx_encode_fmt.cam.camParameters.specialVehicleContainer.u.emergencyContainer.emergencyPriority));
    }

    return;

}

static int cam_decode(uint8_t *p_rx_payload, int rx_payload_len, itsg5_rx_info_t *p_itsg5_rx_info, int security_protocol, int evaluation_mode)
{
    // Evaluation zone: ---------------- ---------------- ---------------- ----------------
    struct timespec start_time, end_time;
    // ---------------- ---------------- ---------------- ---------------- ----------------

    ITSMsgCodecErr err;
    ItsPduHeader *p_rx_decode_hdr = NULL;
    CAM *p_rx_decode_cam = NULL;
    CAM_V2 *p_rx_decode_cam_v2 = NULL;
    int result;

    /* Allocate a buffer for restoring the decode error information if needed. */
    err.msg_size = 256;
    err.msg = calloc(1, err.msg_size);

    if (err.msg == NULL) {
        printf("Cannot allocate memory for error message buffer.\n");
        return -1;
    }

    /* Determine and decode the content in RX payload. */
    result = itsmsg_decode(&p_rx_decode_hdr, p_rx_payload, rx_payload_len, &err);

    /* Check whether this is a ITS message. */
    if (result > 0 && p_rx_decode_hdr != NULL) {
        /* Check whether this is a ITS CAM message. */
        if (p_rx_decode_hdr->messageID == CAM_Id) {
            /* Display ITS message version. */
            // EVAL COMMENT:
            //printf("ITS msg protocol version: v%d\n", p_rx_decode_hdr->protocolVersion);

            /* Mapping data format base on protocol version. */
            switch (p_rx_decode_hdr->protocolVersion) {
                case 1:
                    /* Convert to version 1 CAM data format. */
                    p_rx_decode_cam = (CAM *)p_rx_decode_hdr;

                    /* Extract other message element, the example only extract contents of newest version Unex supported */
                    printf("[ Received CAM from station %u ]\n", p_rx_decode_cam->header.stationID);
                    break;
                case 2:
                    /* Convert to version 2 CAM data format. */
                    p_rx_decode_cam_v2 = (CAM_V2 *)p_rx_decode_hdr;

                    /* Check CAM msg permission */
                    if (p_itsg5_rx_info->security.status == ITSG5_DECAP_VERIFIED_PKT) {
                        result = cam_check_msg_permission(p_rx_decode_cam_v2, p_itsg5_rx_info->security.ssp, p_itsg5_rx_info->security.ssp_len);
                        printf("Check msg permissions: ");
                        if (IS_SUCCESS(result)) {
                            printf("trustworthy\n");
                        }
                        else {
                            printf("untrustworthy\n");
                        }
                    }

                    /* Display the decoded CAM content. */
                    // EVAL COMMENT:
                    /*printf("[ Received CAM from station %u ]\n", p_rx_decode_cam_v2->header.stationID);
                    printf("\tStation type: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.stationType);
                    printf("\tgenerationDeltaTime: %d\n", p_rx_decode_cam_v2->cam.generationDeltaTime);
                    printf("\tLatitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.latitude);
                    printf("\tLongitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.longitude);
                    printf("\tAltitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue);*/
                

                    /* 
                    * ------------------------------------------------------------------------------------------------------------------------------------------------
                    * Decapsulation code START -----------------------------------------------------------------------------------------------------------------------
                    * ------------------------------------------------------------------------------------------------------------------------------------------------
                    */

                    if(security_protocol == 1) {

                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ DEBUG PRINTS
                        // ------------------------------------------------------------------------------------------------------------------------

                        //printf("DLAPP --------------------------------------------------\n");

                        // Debug prints to see incoming data:
                        //EVAL COMMENT:
                        /*printf("\nReceived CAM encoded data:");
                        dump_mem(p_rx_payload, rx_payload_len); // Prints data in hexa
                        printf("Received CAM size: %d\n\n", rx_payload_len);*/

                        // Payload
                        /*
                        printf("p_rx_payload elements:\n");
                        for (int i = 0; i < rx_payload_len; i++) {
                            printf("%02x ", p_rx_payload[i]);
                        }
                        printf("\n");
                        */

                        // Get pathPoints total and objects 
                        int numPathPoints = p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory.count;
                        struct PathPoint* pathPoints = p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory.tab;

                        // See path points
                        /*
                        printf("Path Points:\n");
                        for (int i = 0; i < numPathPoints; i++) {
                            printf("Path Point %d:\n", i + 1);
                            printf("Delta Latitude: %d\n", pathPoints[i].pathPosition.deltaLatitude);
                            printf("Delta Longitude: %d\n", pathPoints[i].pathPosition.deltaLongitude);
                            printf("Delta Altitude: %d\n", pathPoints[i].pathPosition.deltaAltitude);
                            printf("\n");
                        }
                        */

                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ DLAPP VERIFICATION START
                        // ------------------------------------------------------------------------------------------------------------------------

                        // For now, hardcoded
                        unsigned char hash_chain_keys[5][32] = {
                            {240, 9, 204, 205, 214, 160, 104, 101, 7, 17, 165, 42, 184, 56, 112, 76, 103, 115, 237, 223, 31, 137, 82, 113, 162, 114, 144, 175, 199, 162, 106, 216},
                            {187, 56, 198, 22, 44, 167, 248, 205, 171, 174, 251, 201, 221, 205, 237, 123, 186, 151, 123, 199, 204, 38, 186, 25, 108, 255, 31, 105, 78, 199, 130, 60},
                            {143, 15, 144, 71, 42, 90, 229, 34, 228, 80, 37, 227, 153, 54, 218, 239, 70, 48, 231, 172, 44, 106, 170, 164, 72, 117, 173, 114, 98, 255, 218, 84},
                            {184, 165, 136, 38, 140, 22, 194, 211, 128, 201, 99, 36, 10, 215, 122, 191, 135, 195, 115, 116, 201, 3, 152, 138, 80, 63, 132, 5, 189, 245, 243, 90},
                            {155, 84, 205, 254, 51, 156, 175, 155, 41, 220, 171, 121, 83, 82, 215, 202, 144, 56, 69, 170, 190, 90, 159, 12, 206, 206, 206, 164, 6, 47, 47, 134}
                        };

                        // ------------------------------------------ //
                        // ------ Fields size (40 bytes total) ------ //
                        // ------------------------------------------ //
                        uint16_t pseudo_id_total_bytes = 20;
                        uint16_t mac_total_bytes = 12;
                        uint16_t random_key_index_total_bytes = 4;
                        uint16_t timestamp_total_bytes = 4;

                        // ----------------------------------------- //
                        // ------------ Indexes to read ------------ //
                        // ----------------------------------------- //

                        // The message was concatenated like this:
                        // pseudoId + mac + randomKeyIndex + timestamp + itsMessage

                        // Read pseudo id
                        uint16_t read_pseudo_id_start = 0;
                        uint16_t read_pseudo_id_end = pseudo_id_total_bytes;

                        // Read mac
                        uint16_t read_mac_start = read_pseudo_id_end;
                        uint16_t read_mac_end = read_mac_start + mac_total_bytes;

                        // Read random key index
                        uint16_t read_random_key_idx_start = read_mac_end;
                        uint16_t read_random_key_idx_end = read_random_key_idx_start + random_key_index_total_bytes;

                        // Read timestamp
                        uint16_t read_timestamp_start = read_random_key_idx_end;
                        uint16_t read_timestamp_end = read_timestamp_start + timestamp_total_bytes;

                        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
                        if(evaluation_mode == 1) {
                            // Get initial timestamp
                            clock_gettime(CLOCK_MONOTONIC, &start_time);
                        }
                        // ---------------- ---------------- ---------------- ---------------- ----------------

                        // ----------------------------------------- //
                        // ---- Retrieve all 40 security bytes. ---- //
                        // ----------------------------------------- //
                        signed char byteArray[40];

                        for (int i = 0; i < numPathPoints; i++) {
                            PathPoint *pathPoint = &(pathPoints[i]);
                            
                            DeltaReferencePosition *pathPosition = &(pathPoint->pathPosition);
                            
                            int deltaLatitude = pathPoints[i].pathPosition.deltaLatitude;
                            int deltaLongitude = pathPoints[i].pathPosition.deltaLongitude;

                            byteArray[((i + 1) * 4) - 4] = (deltaLatitude >> 8) & 0xFF;     // Get the most significant byte of deltaLatitude
                            byteArray[((i + 1) * 4) - 3] = deltaLatitude & 0xFF;            // Get the least significant byte of deltaLatitude

                            byteArray[((i + 1) * 4) - 2] = (deltaLongitude >> 8) & 0xFF;    // Get the most significant byte of deltaLongitude
                            byteArray[((i + 1) * 4) - 1] = deltaLongitude & 0xFF;           // Get the least significant byte of deltaLongitude
                        }

                        // ------------------------------------------- //
                        // ----- Checks the timestamp's validity ----- //
                        // ------------------------------------------- //

                        signed char ts_bytes_recover[4]; // hardcoded because it is in a switch

                        for (int i = read_timestamp_end - 1; i >= read_timestamp_start; i--) {
                            ts_bytes_recover[read_timestamp_end - 1 - i] = byteArray[i];
                            //printf("\nread tstamp idx: %d, byte-:%d ", read_timestamp_end - 1 - i, ts_bytes_recover[read_timestamp_end - 1 - i]); // DEBUG
                        }
                        
                        int32_t ts = *(int32_t *)ts_bytes_recover;
                        int32_t current_ts = (int32_t)time(NULL);

                        /*printf("Received timestamp: %d\n", ts);
                        printf("System timestamp: %d\n", current_ts);*/

                        // Due to sync difficulties, we have the following condition:
                        if (!(ts == current_ts || ts == current_ts + 1 || ts == current_ts - 1)) {
                            printf("Timestamp not validated!\n"); // Exit.
                        }

                        //printf("\nReading TS bytes");
                        unsigned char ts_bytes[4]; // hardcoded because it is in a switch scope
                        for (uint32_t i = read_timestamp_start; i < read_timestamp_end; i++) {
                            ts_bytes[i - read_timestamp_start] = byteArray[i];
                            //printf("\nread ts idx: %d, byte-:%d ", i - read_timestamp_start, ts_bytes[i - read_timestamp_start]); // DEBUG
                        }
                        //printf("\n"); // DEBUG
                        
                        // ---------------------------- //
                        // --------- Pseudo-id -------- //
                        // ---------------------------- //                    
                        //printf("\nReading PID bytes");
                        unsigned char pid_bytes[20]; // hardcoded because it is in a switch scope
                        for (int i = read_pseudo_id_start; i < read_pseudo_id_end; i++) {
                            pid_bytes[i - read_pseudo_id_start] = byteArray[i];
                            //printf("\nread pid idx: %d, byte-:%d ", i - read_pseudo_id_start, pid_bytes[i - read_pseudo_id_start]); // DEBUG
                        }
                        //printf("\n"); // DEBUG
                        
                        // -------------------------------- //
                        // ----- Verify the signature ----- //
                        // -------------------------------- //

                        /*
                        *  First, is necessary to extract the bytes from each field. The message was concatenated like this:
                        *  pseudoId + mac + randomKeyIndex + timestamp + itsMessage
                        *
                        *  The timestamp is not necessary to extract because it was already previously extracted.
                        */
                        
                        // -----------------
                        // 1. Extract fields
                        // -----------------

                        /** Get MAC bytes **/
                        //printf("\nReading MAC bytes");
                        signed char mac_bytes[12]; // hardcoded because it is in a switch scope
                        for (int i = read_mac_end - 1; i >= read_mac_start; i--) {
                            mac_bytes[read_mac_end - 1 - i] = byteArray[i];
                            //printf("\nread mac idx: %d, byte-:%d ", read_mac_end - 1 - i, mac_bytes[read_mac_end - 1 - i]); // DEBUG
                        }
                        //printf("\n"); // DEBUG


                        /** Get random key index bytes **/
                        //printf("\nReading Random Key Index bytes");
                        signed char random_key_index_bytes[4];  // hardcoded because it is in a switch scope
                        for (int i = read_random_key_idx_end - 1; i >= read_random_key_idx_start; i--) {
                            random_key_index_bytes[read_random_key_idx_end - 1 - i] = byteArray[i];
                            //printf("\nread Random Key Index idx: %d, byte-:%d ", read_random_key_idx_end - 1 - i, random_key_index_bytes[read_random_key_idx_end - 1 - i]); // DEBUG
                        }
                        //printf("\n"); // DEBUG
                        int32_t random_key_index = *(int32_t *)random_key_index_bytes;

                        // Printing all elements of arrays[random_key_index]
                        /*printf("Key %d\n", random_key_index);
                        for (int i = 0; i < sizeof(hash_chain_keys[random_key_index]); i++) {
                            printf("%d ", hash_chain_keys[random_key_index][i]);
                        }
                        printf("\n");*/


                        // -------------------------------- //
                        // -- Get ITS original msg bytes -- //
                        // -------------------------------- //

                        // Remove the LowFrequencyContainer (where the security info was inserted)
                        p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer_option = FALSE;

                        ITSMsgCodecErr err_temp;
                        uint8_t *its_message_bytes  = NULL;
                        uint8_t **temp_buf = &its_message_bytes ;
                        int its_message_bytes_len = 0;

                        /* Allocate a buffer for restoring the decode error information if needed. */
                        err_temp.msg_size = 512;
                        err_temp.msg = calloc(1, err_temp.msg_size);
                        
                        ItsPduHeader *header = (ItsPduHeader *)p_rx_decode_cam_v2;
                        /*printf("Protocol Version: %" PRId32 "\n", header->protocolVersion);
                        printf("Message ID: %" PRId32 "\n", header->messageID);
                        printf("Station ID: %" PRIu32 "\n", header->stationID);*/

                        if (err_temp.msg != NULL) {

                            /* Encode the CAM message. */
                            its_message_bytes_len = itsmsg_encode(temp_buf, (ItsPduHeader *)p_rx_decode_cam_v2, &err_temp);

                            if (its_message_bytes_len <= 0) {
                                printf("itsmsg_encode error v2: %s\n", err_temp.msg);
                            }

                            /*printf("Original CAM encoded data:\n");
                            dump_mem(its_message_bytes , its_message_bytes_len);

                            printf("its_message_bytes  elements:\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%02x ", its_message_bytes [i]);
                            }
                            printf("\n");*/

                            /*printf("Original CAM encoded data: (its_message_bytes  elements as int8_t):\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%d ", (int8_t)its_message_bytes [i]);
                            }
                            printf("\n");

                            printf("Original CAM encoded data: (its_message_bytes  elements as uint8_t):\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%d ", its_message_bytes [i]);
                            }
                            printf("\n");*/

                            //EVAL COMMENT:
                            /*printf("\nORIGINAL CAM encoded data:");
                            dump_mem(its_message_bytes, its_message_bytes_len); // Prints data in hexa

                            printf("ORIGINAL CAM size: %d\n\n", its_message_bytes_len);*/


                            free(err_temp.msg);
                        }
                        else {
                            printf("Cannot allocate memory for error message buffer.\n");
                        }
                        
                        
                        /** Concatenate the byte arrays to verify its integrity and authenticity **/

                        // Calculate the total length of the resulting byte array
                        size_t total_length = sizeof(pid_bytes) + its_message_bytes_len + sizeof(ts_bytes);
                        
                        // Create the resulting byte array
                        unsigned char *msg = (unsigned char*)malloc(total_length);
                        
                        // Concatenate the byte arrays
                        size_t index = 0;
                        
                        // Copy bytes from pid_bytes to the result array
                        for (size_t i = 0; i < sizeof(pid_bytes); i++) {
                            msg[index++] = pid_bytes[i];
                        }
                        
                        // Copy bytes from its_message_bytes to the result array
                        for (size_t i = 0; i < its_message_bytes_len; i++) {
                            msg[index++] = its_message_bytes[i];
                        }
                        
                        // Copy bytes from ts_bytes to the result array
                        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
                            msg[index++] = ts_bytes[i];
                        }

                        // Print the content of the 'msg' byte array
                        /*printf("msg content:\n");
                        for (size_t i = 0; i < total_length; i++) {
                            //printf("msg content[%d] : %d\n", i, msg[i]);
                            printf("%d ", msg[i]);
                        }
                        printf("\n");*/

                        
                        unsigned char hash[32]; // Output hash will be stored here
                        
                        size_t hashlen = hmac_sha256(hash_chain_keys[random_key_index], sizeof(hash_chain_keys[random_key_index]), msg, total_length, hash, sizeof(hash));
                        
                        //printf("Integrity validation:\n");
                        bool valid = true;
                        for (size_t i = 0; i < 12; i++) {
                            /*printf("calculated %d ", (signed char) hash[hashlen - i -1]);
                            printf("received %d \n", mac_bytes[i]);*/

                            if((signed char) hash[hashlen - i -1] != mac_bytes[i]) {
                                valid = false;
                            }
                                
                        }

                        if(!valid) {
                            printf("Message not verified!");
                        }
                        
                        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
                        if(evaluation_mode == 1) {
                            // Get final timestamp in milliseconds precision
                            clock_gettime(CLOCK_MONOTONIC, &end_time);
                            
                            // Calculate the time difference with more precision
                            double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                                (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
                            
                            printf("%.9lf\n", elapsed_time);
                        }
                        // ---------------- ---------------- ---------------- ---------------- ----------------

                        //printf("\n");
                
                        free(msg);

                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ DLAPP VERIFICATION END
                        // ------------------------------------------------------------------------------------------------------------------------


                    } else if(security_protocol == 2) {
                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ DEBUG PRINTS
                        // ------------------------------------------------------------------------------------------------------------------------

                        // printf("MFSPV --------------------------------------------------\n");

                        // Debug prints to see incoming data:
                        //EVAL COMMENT:
                        /*printf("\nReceived CAM encoded data:");
                        dump_mem(p_rx_payload, rx_payload_len); // Prints data in hexa
                        printf("Received CAM size: %d\n\n", rx_payload_len);*/

                        // Payload
                        /*
                        printf("p_rx_payload elements:\n");
                        for (int i = 0; i < rx_payload_len; i++) {
                            printf("%02x ", p_rx_payload[i]);
                        }
                        printf("\n");
                        */

                        // Payload as int8
                        /*printf("p_rx_payload elements as int8_t:\n");
                        for (int i = 0; i < rx_payload_len; i++) {
                            printf("%d ", (int8_t)p_rx_payload[i]);
                        }
                        printf("\n\n");
                        printf("rx_payload_len: %d\n", rx_payload_len);*/
    
                        // Get pathPoints total and objects 
                        int numPathPoints = p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory.count;
                        struct PathPoint* pathPoints = p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer.u.basicVehicleContainerLowFrequency.pathHistory.tab;

                        // See path points
                        /*
                        printf("Path Points:\n");
                        for (int i = 0; i < numPathPoints; i++) {
                            printf("Path Point %d:\n", i + 1);
                            printf("Delta Latitude: %d\n", pathPoints[i].pathPosition.deltaLatitude);
                            printf("Delta Longitude: %d\n", pathPoints[i].pathPosition.deltaLongitude);
                            printf("Delta Altitude: %d\n", pathPoints[i].pathPosition.deltaAltitude);
                            printf("\n");
                        }
                        */

                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ MFSPV VERIFICATION START
                        // ------------------------------------------------------------------------------------------------------------------------
    
                        // For now, the following data is hardcoded
                        unsigned char regional_key[32] = {240, 9, 204, 205, 214, 160, 104, 101, 7, 17, 165, 42, 184, 56, 112, 76, 103, 115, 237, 223, 31, 137, 82, 113, 162, 114, 144, 175, 199, 162, 106, 216};
        
                        // ------------------------------------------ //
                        // ------ Fields size (40 bytes total) ------ //
                        // ------------------------------------------ //
                        uint16_t pseudo_id_total_bytes = 20;
                        uint16_t mac_total_bytes = 20;
                        uint16_t timestamp_total_bytes = 4;

                        // ----------------------------------------- //
                        // ------------ Indexes to read ------------ //
                        // ----------------------------------------- //

                        // The message was concatenated like this:
                        // timestamp + mac + pseudoId + itsMessage

                        // Read timestamp
                        uint16_t read_timestamp_start = 0;
                        uint16_t read_timestamp_end = timestamp_total_bytes;

                        // Read mac
                        uint16_t read_mac_start = read_timestamp_end;
                        uint16_t read_mac_end = read_mac_start + mac_total_bytes;

                        // Read pseudo id
                        uint16_t read_pseudo_id_start = read_mac_end;
                        uint16_t read_pseudo_id_end = read_pseudo_id_start + pseudo_id_total_bytes;

                        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
                        if(evaluation_mode == 1) {
                            // Get initial timestamp
                            clock_gettime(CLOCK_MONOTONIC, &start_time);
                        }
                        // ---------------- ---------------- ---------------- ---------------- ----------------

                        // ----------------------------------------- //
                        // ---- Retrieve all 44 security bytes. ---- //
                        // ----------------------------------------- //
                        signed char byteArray[44];

                        for (int i = 0; i < numPathPoints; i++) {
                            PathPoint *pathPoint = &(pathPoints[i]);
                            
                            DeltaReferencePosition *pathPosition = &(pathPoint->pathPosition);
                            
                            int deltaLatitude = pathPoints[i].pathPosition.deltaLatitude;
                            int deltaLongitude = pathPoints[i].pathPosition.deltaLongitude;

                            byteArray[((i + 1) * 4) - 4] = (deltaLatitude >> 8) & 0xFF;     // Get the most significant byte of deltaLatitude
                            byteArray[((i + 1) * 4) - 3] = deltaLatitude & 0xFF;            // Get the least significant byte of deltaLatitude

                            byteArray[((i + 1) * 4) - 2] = (deltaLongitude >> 8) & 0xFF;    // Get the most significant byte of deltaLongitude
                            byteArray[((i + 1) * 4) - 1] = deltaLongitude & 0xFF;           // Get the least significant byte of deltaLongitude
                        }
                        
                        // ------------------------------------------- //
                        // ----- Checks the timestamp's validity ----- //
                        // ------------------------------------------- //

                        signed char ts_bytes_recover[4]; // hardcoded because it is in a switch

                        for (int i = read_timestamp_end - 1; i >= read_timestamp_start; i--) {
                            ts_bytes_recover[read_timestamp_end - 1 - i] = byteArray[i];
                            //printf("read tstamp idx: %d, byte-:%d\n", read_timestamp_end - 1 - i, ts_bytes_recover[read_timestamp_end - 1 - i]); // DEBUG
                        }
                        
                        int32_t ts = *(int32_t *)ts_bytes_recover;
                        int32_t current_ts = (int32_t)time(NULL);
                        
                        /*printf("Received timestamp: %d\n", ts);
                        printf("System timestamp: %d\n", current_ts);*/

                        if (!(ts == current_ts || ts == current_ts + 1 || ts == current_ts - 1)) {
                            printf("Timestamp not validated!\n"); // Exit.
                        }

                        //printf("\nReading TS bytes");
                        unsigned char ts_bytes[4]; // hardcoded because it is in a switch scope
                        for (uint32_t i = read_timestamp_start; i < read_timestamp_end; i++) {
                            ts_bytes[i - read_timestamp_start] = byteArray[i];
                            //printf("\nread ts idx: %d, byte-:%d ", i - read_timestamp_start, ts_bytes[i - read_timestamp_start]); // DEBUG
                        }
                        //printf("\n"); // DEBUG

                        // ---------------------------- //
                        // --------- Pseudo-id -------- //
                        // ---------------------------- //                    
                        //printf("\nReading PID bytes");
                        unsigned char pid_bytes[20]; // hardcoded because it is in a switch scope
                        for (int i = read_pseudo_id_start; i < read_pseudo_id_end; i++) {
                            pid_bytes[i - read_pseudo_id_start] = byteArray[i];
                            //printf("\nread pid idx: %d, byte-:%d ", i - read_pseudo_id_start, pid_bytes[i - read_pseudo_id_start]); // DEBUG
                        }
                        //printf("\n"); // DEBUG

                        // -------------------------------- //
                        // ----- Verify the signature ----- //
                        // -------------------------------- //

                        /*
                        *  First, is necessary to extract the bytes from each field. The message was concatenated like this:
                        *  timestamp + mac + pseudoId + itsMessage
                        *
                        *  The timestamp is not necessary to extract because it was already previously extracted.
                        */
                        
                        // -----------------
                        // 1. Extract fields
                        // -----------------

                        /** Get MAC bytes **/
                        //printf("\nReading MAC bytes");
                        signed char mac_bytes[20]; // hardcoded because it is in a switch scope
                        for (int i = read_mac_end - 1; i >= read_mac_start; i--) {
                            mac_bytes[read_mac_end - 1 - i] = byteArray[i];
                            //printf("\nread mac idx: %d, byte-:%d ", read_mac_end - 1 - i, mac_bytes[read_mac_end - 1 - i]); // DEBUG
                        }
                        //printf("\n"); // DEBUG

                        // -------------------------------- //
                        // -- Get ITS original msg bytes -- //
                        // -------------------------------- //

                        // Remove the LowFrequencyContainer (where the security info was inserted)
                        p_rx_decode_cam_v2->cam.camParameters.lowFrequencyContainer_option = FALSE;

                        ITSMsgCodecErr err_temp;
                        uint8_t *its_message_bytes  = NULL;
                        uint8_t **temp_buf = &its_message_bytes ;
                        int its_message_bytes_len = 0;

                        /* Allocate a buffer for restoring the decode error information if needed. */
                        err_temp.msg_size = 512;
                        err_temp.msg = calloc(1, err_temp.msg_size);
                        
                        ItsPduHeader *header = (ItsPduHeader *)p_rx_decode_cam_v2;
                        /*printf("Protocol Version: %" PRId32 "\n", header->protocolVersion);
                        printf("Message ID: %" PRId32 "\n", header->messageID);
                        printf("Station ID: %" PRIu32 "\n", header->stationID);*/

                        if (err_temp.msg != NULL) {

                            /* Encode the CAM message. */
                            its_message_bytes_len = itsmsg_encode(temp_buf, (ItsPduHeader *)p_rx_decode_cam_v2, &err_temp);

                            if (its_message_bytes_len <= 0) {
                                printf("itsmsg_encode error v2: %s\n", err_temp.msg);
                            }

                            /*printf("Original CAM encoded data:\n");
                            dump_mem(its_message_bytes , its_message_bytes_len);

                            printf("its_message_bytes  elements:\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%02x ", its_message_bytes [i]);
                            }
                            printf("\n");*/

                            /*printf("Original CAM encoded data: (its_message_bytes  elements as int8_t):\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%d ", (int8_t)its_message_bytes [i]);
                            }
                            printf("\n");

                            printf("Original CAM encoded data: (its_message_bytes  elements as uint8_t):\n");
                            for (int i = 0; i < its_message_bytes_len; i++) {
                                printf("%d ", its_message_bytes [i]);
                            }
                            printf("\n");*/

                            //EVAL COMMENT:
                            /*printf("\nORIGINAL CAM encoded data:");
                            dump_mem(its_message_bytes, its_message_bytes_len); // Prints data in hexa

                            printf("ORIGINAL CAM size: %d\n\n", its_message_bytes_len);*/

                            free(err_temp.msg);
                        }
                        else {
                            printf("Cannot allocate memory for error message buffer.\n");
                        }

                        /** Concatenate the byte arrays to verify its integrity and authenticity **/

                        // Calculate the total length of the resulting byte array
                        size_t total_length = sizeof(pid_bytes) + sizeof(regional_key) + its_message_bytes_len + sizeof(ts_bytes);
                        
                        // Create the resulting byte array
                        unsigned char *msg = (unsigned char*)malloc(total_length);
                        
                        // Concatenate the byte arrays
                        size_t index = 0;
                        
                        // Copy bytes from pid_bytes to the result array
                        for (size_t i = 0; i < sizeof(pid_bytes); i++) {
                            msg[index++] = pid_bytes[i];
                        }

                        // Copy bytes from regional key to the result array
                        for (size_t i = 0; i < sizeof(regional_key); i++) {
                            msg[index++] = regional_key[i];
                        }
                        
                        // Copy bytes from its_message_bytes to the result array
                        for (size_t i = 0; i < its_message_bytes_len; i++) {
                            msg[index++] = its_message_bytes[i];
                        }
                        
                        // Copy bytes from ts_bytes to the result array
                        for (size_t i = 0; i < sizeof(ts_bytes); i++) {
                            msg[index++] = ts_bytes[i];
                        }
                        
                        // Calculate the SHA-256 hash
                        unsigned char hash[SHA256_DIGEST_LENGTH];
                        SHA256(msg, total_length, hash);

                        //printf("Integrity validation:\n");
                        bool valid = true;
                        for (size_t i = 0; i < 20; i++) {
                            /*printf("calculated %d ", (signed char) hash[SHA256_DIGEST_LENGTH - i -1]);
                            printf("received %d \n", mac_bytes[i]);*/

                            if((signed char) hash[SHA256_DIGEST_LENGTH - i -1] != mac_bytes[i]) {
                                valid = false;
                            }
                                
                        }

                        if(!valid) {
                            printf("Message not verified!");
                        }

                        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
                        if(evaluation_mode == 1) {
                            // Get final timestamp in milliseconds precision
                            clock_gettime(CLOCK_MONOTONIC, &end_time);
                            
                            // Calculate the time difference with more precision
                            double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                                (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
                            
                            printf("%.9lf\n", elapsed_time);
                        }
                        // ---------------- ---------------- ---------------- ---------------- ----------------

                        //printf("\n");
                
                        free(msg);


                        // ------------------------------------------------------------------------------------------------------------------------
                        // ------------------------------------------------------------------------------------------------------------------------ MSFPV VERIFICATION END
                        // ------------------------------------------------------------------------------------------------------------------------
                    }
                    
                    /* 
                    * ------------------------------------------------------------------------------------------------------------------------------------------------
                    * Decapsulation code END -------------------------------------------------------------------------------------------------------------------------
                    * ------------------------------------------------------------------------------------------------------------------------------------------------
                    */



                    fprintf(rx_log_file, "[%s]:\n", getCurrentTime());
                    fprintf(rx_log_file,"[ Received CAM from station %u ]\n", p_rx_decode_cam_v2->header.stationID);
                    fprintf(rx_log_file,"\tStation type: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.stationType);
                    fprintf(rx_log_file,"\tgenerationDeltaTime: %d\n", p_rx_decode_cam_v2->cam.generationDeltaTime);
                    fprintf(rx_log_file,"\tLatitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.latitude);
                    fprintf(rx_log_file,"\tLongitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.longitude);
                    fprintf(rx_log_file,"\tAltitude: %d\n", p_rx_decode_cam_v2->cam.camParameters.basicContainer.referencePosition.altitude.altitudeValue);
                    rx_counter++;

                    break;

                default:

                    printf("Received unsupported CAM protocol version: %d\n", p_rx_decode_hdr->protocolVersion);
                    break;
            }
        }
        else {
            printf("Received unrecognized ITS message type: %d\n", p_rx_decode_hdr->messageID);
        }

        /* Release the decode message buffer. */
        itsmsg_free(p_rx_decode_hdr);
    }
    else {
        printf("Unable to decode RX packet: %s\n", err.msg);
    }

    /* Release the allocated error message buffer. */
    free(err.msg);

    return 0;
}

static void cam_recv(itsg5_caster_comm_config_t *config, int security_protocol, int evaluation_mode)
{
    itsg5_rx_info_t rx_info = {0};
    uint8_t rx_buf[ITSG5_CASTER_PKT_SIZE_MAX];
    size_t len;
    int ret;
    itsg5_caster_handler_t handler = ITSG5_INVALID_CASTER_HANDLER;

    // Caster creation to message receiving
    ret = itsg5_caster_create(&handler, config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return;
    }

    while (app_running) {
        // EVAL COMMENT
        //printf("-----------------------\n");

        // Waiting a CAM message
        ret = itsg5_caster_rx(handler, &rx_info, rx_buf, sizeof(rx_buf), &len);

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        struct timespec start_time, end_time;
        // ---------------- ---------------- ---------------- ---------------- ----------------

        if (IS_SUCCESS(ret)) {
            // EVAL COMMENT
            //printf("Received %zu bytes!\n", len);
            fprintf(rx_log_file, "Received %zu bytes!\n", len);

            /* Display ITS-G5 RX information */
            dump_rx_info(&rx_info);

            // Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if(evaluation_mode == 2) {
                // Get initial timestamp
                clock_gettime(CLOCK_MONOTONIC, &start_time);
            }
            // ---------------- ---------------- ---------------- ---------------- ----------------

            /* Try to decode the received message. */
            cam_decode(rx_buf, (int)len, &rx_info, security_protocol, evaluation_mode);

            // Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if(evaluation_mode == 2) {
                // Get final timestamp in milliseconds precision
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                
                // Calculate the time difference with more precision
                double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                    (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
                
                printf("%.9lf\n", elapsed_time);
            }
            // ---------------- ---------------- ---------------- ---------------- ----------------

        }
        else {
            printf("Failed to receive data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
        }
        fflush(stdout);
        fflush(rx_log_file);
        
    }

    // Caster release
    itsg5_caster_release(handler);
}

static void cam_send(itsg5_caster_comm_config_t *config, bool is_secured, int security_protocol, int evaluation_mode)
{
    
    uint8_t *tx_buf = NULL;
    int tx_buf_len = 0;
    int ret;
    poti_fix_data_t fix_data = {0};
    poti_gnss_info_t gnss_info = {0};
    itsg5_tx_info_t tx_info = {0}; /* According to C99, all tx_info members will be set to 0 */
    itsg5_caster_handler_t handler = ITSG5_INVALID_CASTER_HANDLER;
    poti_handler_t poti_handler = POTI_INVALID_CASTER_HANDLER;

    ret = itsg5_caster_create(&handler, config);
    if (!IS_SUCCESS(ret)) {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return;
    }

    poti_comm_config_t poti_config = {.ip = config->ip};
    ret = poti_caster_create(&poti_handler, &poti_config);
    if (!IS_SUCCESS(ret)) {
        printf("Fail to create POTI caster, ret:%d!\n", ret);
        return;
    }
    
    // Evaluation zone: ---------------- ---------------- ---------------- ----------------
    struct timespec start_time, end_time;
    // ---------------- ---------------- ---------------- ---------------- ----------------

    while (app_running) {
        // EVAL COMMENT
        //printf("-----------------------\n");

        /* Get GNSS fix data from POTI caster service */
        ret = poti_caster_fix_data_rx(poti_handler, &fix_data);
        
        if (ret != 0) {
            printf("Fail to receive GNSS fix data from POTI caster service, %d\n", ret);
            /* Waiting for POTI caster service startup */
            usleep(1000000);
            continue;
        }
        else if (fix_data.mode < POTI_FIX_MODE_2D) {
            printf("GNSS not fix, fix mode: %d\n", fix_data.mode);

            /* Optional, APIs for users to get more GNSS information */
            ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
            if (IS_SUCCESS(ret)) {
                printf("GNSS antenna status:%d, time sync status: %d\n", gnss_info.antenna_status, gnss_info.time_sync_status);
            }

            /* The interval of get GNSS fix data is depend on GNSS data rate */
            usleep(100000);
            continue;
        }

        /* Optional, NAN value check for GNSS data */
        if ((isnan(fix_data.latitude) == 1) || (isnan(fix_data.longitude) == 1) || (isnan(fix_data.altitude) == 1) || (isnan(fix_data.horizontal_speed) == 1) || (isnan(fix_data.course_over_ground) == 1)) {
            printf("GNSS fix data has NAN value, latitude: %f, longitude : %f, altitude : %f, horizontal speed : %f, course over ground : %f\n", fix_data.latitude, fix_data.longitude, fix_data.altitude, fix_data.horizontal_speed, fix_data.course_over_ground);
            continue;
        }
        
        ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
        // EVAL COMMENT
        /*if (IS_SUCCESS(ret)) {
            printf("Access layer time sync state: %d , unsync times: %d\n", gnss_info.acl_time_sync_state, gnss_info.acl_unsync_times);
        }*/

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 2) {
            // Get initial timestamp
            clock_gettime(CLOCK_MONOTONIC, &start_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------

        cam_encode(&tx_buf, &tx_buf_len, &fix_data, security_protocol, evaluation_mode);

        // Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if(evaluation_mode == 2) {
            // Get final timestamp in milliseconds precision
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            
            // Calculate the time difference with more precision
            double elapsed_time = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                                (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
            
            printf("%.9lf\n", elapsed_time);
        }
        // ---------------- ---------------- ---------------- ---------------- ----------------

        if (tx_buf != NULL) {
            //EVAL COMMENT:
            /*printf("CAM encoded data:\n");
            dump_mem(tx_buf, tx_buf_len);*/
            set_tx_info(&tx_info, is_secured);

            ret = itsg5_caster_tx(handler, &tx_info, tx_buf, (size_t)tx_buf_len);
            if (IS_SUCCESS(ret)) {
                // EVAL COMMENT
                //printf("Transmitted %zu bytes!\n", tx_buf_len);
                fprintf(tx_log_file, "\tTransmitted %zu bytes!\n", tx_buf_len);
            }
            else {
                printf("Failed to transmit data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
            }
            itsmsg_buf_free(tx_buf);
        }
        fflush(stdout);
        fflush(tx_log_file);
        

        /* This time interval is an example only, the message generation interval please refer to ETSI EN 302 637-2 */
        sleep(1);
    }

    itsg5_caster_release(handler);
    poti_caster_release(poti_handler);
}

static int cam_check_msg_permission(CAM_V2 *p_cam_msg, uint8_t *p_ssp, uint8_t ssp_len)
{
    int rc = 0, fbs;

    if (ssp_len < CAM_SSP_LEN) {
        rc = -1;
        printf("Err: SSP length[%d] is not enough\n", ssp_len);
        goto FAILURE;
    }

    if (p_cam_msg->cam.camParameters.specialVehicleContainer_option) {
        /*
        *   For example, only check emergencyContainer
        *   Please refer to ETSI EN 302 637-2 to check related SSP item
        */
        switch (p_cam_msg->cam.camParameters.specialVehicleContainer.choice) {
            case SpecialVehicleContainer_emergencyContainer:
                if (!IS_CAM_SSP_VALID(EMERGENCY, p_ssp[1])) {
                    printf("Err: certificate not allowed to sign EMERGENCY\n");
                    rc = -1;
                    goto FAILURE;
                }

                if (p_cam_msg->cam.camParameters.specialVehicleContainer.u.emergencyContainer.emergencyPriority_option) {
                    fbs = asn1_bstr_ffs(&(p_cam_msg->cam.camParameters.specialVehicleContainer.u.emergencyContainer.emergencyPriority));
                    switch (fbs) {
                        case EmergencyPriority_requestForRightOfWay:
                            if (!IS_CAM_SSP_VALID(REQUEST_FOR_RIGHT_OF_WAY, p_ssp[2])) {
                                printf("Err: certificate not allowed to sign REQUEST_FOR_RIGHT_OF_WAY\n");
                                rc = -1;
                                goto FAILURE;
                            }
                            break;
                        case EmergencyPriority_requestForFreeCrossingAtATrafficLight:
                            if (!IS_CAM_SSP_VALID(REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT, p_ssp[2])) {
                                printf("Err: certificate not allowed to sign REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT\n");
                                rc = -1;
                                goto FAILURE;
                            }
                            break;
                    }
                }
                break;
            default:
                // nothing
                break;
        }
    }

FAILURE:
    return rc;
}

static void dump_mem(void *data, int len)
{
    int count;
    unsigned char *p = (unsigned char *)data;
    for (count = 0; count < len; count++) {
        if (count % 16 == 0)
            printf("\n");

        printf("%02X ", p[count]);
    }
    printf("\n\n");
}

static void dump_rx_info(itsg5_rx_info_t *rx_info)
{
    struct tm *timeinfo;
    char buffer[80];
    time_t t;

    t = rx_info->timestamp.tv_sec;
    timeinfo = localtime(&t);
    strftime(buffer, 80, "%Y%m%d%H%M%S", timeinfo);
    // EVAL COMMENT
    /*printf("timestamp:%s\n", buffer);
    printf("rssi:%hd\n", rx_info->rssi);
    printf("data rate:%hu\n", rx_info->data_rate);
    printf("remain hop:%hu\n", rx_info->remain_hop_limit);
    printf("decap status:%d\n", rx_info->security.status);*/
    switch (rx_info->security.status) {
        case ITSG5_DECAP_VERIFIED_PKT:
            printf("\tSecurity status: this packet is verified\n");
            printf("\tITS-AID: %u\n", rx_info->security.its_aid);
            printf("\tssp_len = %hu\n", rx_info->security.ssp_len);
            for (uint8_t i = 0; i < rx_info->security.ssp_len; i++) {
                printf("\tssp[%hu]=%hu\n", i, rx_info->security.ssp[i]);
            }
            break;
        case ITSG5_DECAP_UNVERIFIABLE_PKT:
            printf("\tSecurity status:  this packet is untrustworthy\n");
            break;
        case ITSG5_DECAP_INVALID_FMT:
            printf("\tSecurity status: decapsulation error (%d), the payload content is invalid\n", rx_info->security.status);
            break;
        default:
            // Eval comment
            //printf("\tSecurity status: other (%d)\n", rx_info->security.status);
            break;
    }
    return;
}

static void set_tx_info(itsg5_tx_info_t *tx_info, bool is_secured)
{
    /* set data rate*/
    tx_info->data_rate_is_present = true;
    tx_info->data_rate = 12; /* 12 (500kbps) = 6 (Mbps) */

    if (is_secured) {
        /* set security*/
        tx_info->security_is_present = true;
        /*
        * Assign CAM service specific permissions according to the actual content in payload.
        * Please refer to ETSI TS 103 097, ETSI EN 302 637-2 for more details.
        * Please refer to Unex-APG-ETSI-GN-BTP for more information of build-in certificates
        */
        /* SSP Version control */
        tx_info->security.ssp[0] = 0x0;
        /* Service-specific parameter */
        tx_info->security.ssp[1] = EMERGENCY; /* Emergency container */
        tx_info->security.ssp[2] = REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT; /* EmergencyPriority */
        tx_info->security.ssp_len = 3;
    }

    return;
}

/**
 * function_example - Function example
 *
 * @param   [in]    input       Example input.
 * @param   [out]   *p_output   Example output.
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 *
 */
static int32_t cam_set_semi_axis_length(double meter)
{
    /*
     * According to ETSI TS 102 894-2 V1.2.1 (2014-09)
     * The value shall be set to:
     * 1 if the accuracy is equal to or less than 1 cm,
     * n (n > 1 and n < 4 093) if the accuracy is equal to or less than n cm,
     * 4 093 if the accuracy is equal to or less than 4 093 cm,
     * 4 094 if the accuracy is out of range, i.e. greater than 4 093 cm,
     * 4 095 if the accuracy information is unavailable.
     */
    double centimeter;
    int32_t value;

    centimeter = meter * 100.0;

    if (centimeter < 1.0) {
        value = 1;
    }
    else if (centimeter > 1.0 && centimeter < 4093.0) {
        value = (int32_t)centimeter;
    }
    else {
        value = 4094;
    }

    return value;
}

/**
 * function_example - Function example
 *
 * @param   [in]    input       Example input.
 * @param   [out]   *p_output   Example output.
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 *
 */
static int32_t cam_set_heading_value(double degree)
{
    int32_t value;

    if (isnan(degree) == 1) {
        value = 3601;
    }
    else {
        value = degree * 10;
    }

    return value;
}

/**
 * function_example - Function example
 *
 * @param   [in]    input       Example input.
 * @param   [out]   *p_output   Example output.
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 *
 */
static int32_t cam_set_altitude_confidence(double metre)
{
    /*
	 * According to ETSI TS 102 894-2 V1.2.1 (2014-09)
	 * Absolute accuracy of a reported altitude value of a geographical point for a predefined
	 * confidence level (e.g. 95 %). The required confidence level is defined by the
	 * corresponding standards applying the usage of this DE.
	 * The value shall be set to:
	 * 	0 if the altitude accuracy is equal to or less than 0,01 metre
	 * 	1 if the altitude accuracy is equal to or less than 0,02 metre
	 * 	2 if the altitude accuracy is equal to or less than 0,05 metre
	 * 	3 if the altitude accuracy is equal to or less than 0,1 metre
	 * 	4 if the altitude accuracy is equal to or less than 0,2 metre
	 * 	5 if the altitude accuracy is equal to or less than 0,5 metre
	 * 	6 if the altitude accuracy is equal to or less than 1 metre
	 * 	7 if the altitude accuracy is equal to or less than 2 metres
	 * 	8 if the altitude accuracy is equal to or less than 5 metres
	 * 	9 if the altitude accuracy is equal to or less than 10 metres
	 * 	10 if the altitude accuracy is equal to or less than 20 metres
	 * 	11 if the altitude accuracy is equal to or less than 50 metres
	 * 	12 if the altitude accuracy is equal to or less than 100 metres
	 * 	13 if the altitude accuracy is equal to or less than 200 metres
	 * 	14 if the altitude accuracy is out of range, i.e. greater than 200 metres
	 * 	15 if the altitude accuracy information is unavailable
	 */

    int32_t enum_value;

    if (metre <= 0.01) {
        enum_value = AltitudeConfidence_alt_000_01;
    }
    else if (metre <= 0.02) {
        enum_value = AltitudeConfidence_alt_000_02;
    }
    else if (metre <= 0.05) {
        enum_value = AltitudeConfidence_alt_000_05;
    }
    else if (metre <= 0.1) {
        enum_value = AltitudeConfidence_alt_000_10;
    }
    else if (metre <= 0.2) {
        enum_value = AltitudeConfidence_alt_000_20;
    }
    else if (metre <= 0.5) {
        enum_value = AltitudeConfidence_alt_000_50;
    }
    else if (metre <= 1.0) {
        enum_value = AltitudeConfidence_alt_001_00;
    }
    else if (metre <= 2.0) {
        enum_value = AltitudeConfidence_alt_002_00;
    }
    else if (metre <= 5.0) {
        enum_value = AltitudeConfidence_alt_005_00;
    }
    else if (metre <= 10.0) {
        enum_value = AltitudeConfidence_alt_010_00;
    }
    else if (metre <= 20.0) {
        enum_value = AltitudeConfidence_alt_020_00;
    }
    else if (metre <= 50.0) {
        enum_value = AltitudeConfidence_alt_050_00;
    }
    else if (metre <= 100.0) {
        enum_value = AltitudeConfidence_alt_100_00;
    }
    else if (metre <= 200.0) {
        enum_value = AltitudeConfidence_alt_200_00;
    }
    else {
        enum_value = AltitudeConfidence_outOfRange;
    }

    return enum_value;
}

/**
 * function_example - Function example
 *
 * @param   [in]    input       Example input.
 * @param   [out]   *p_output   Example output.
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 *
 */
static int32_t cam_set_heading_confidence(double degree)
{
    /*
	 * According to ETSI TS 102 894-2 V1.2.1 (2014-09)
	 *
	 * The absolute accuracy of a reported heading value for a predefined confidence level
	 * (e.g. 95 %). The required confidence level is defined by the corresponding standards
	 * applying the DE.
	 * The value shall be set to:
	 * ??1 if the heading accuracy is equal to or less than 0,1 degree,
	 * ??n (n > 1 and n < 125) if the heading accuracy is equal to or less than
	 * n ? 0,1 degree,
	 * 	125 if the heading accuracy is equal to or less than 12,5 degrees,
	 * 	126 if the heading accuracy is out of range, i.e. greater than 12,5 degrees,
	 * 	127 if the heading accuracy information is not available.
	 */

    int32_t value;

    if (degree <= 0.1) {
        value = 1;
    }
    else if (degree > 0.1 && degree <= 12.5) {
        value = (int32_t)(degree * 10);
    }
    else {
        value = 126;
    }

    return value;
}

/**
 * function_example - Function example
 *
 * @param   [in]    input       Example input.
 * @param   [out]   *p_output   Example output.
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 *
 */
static int32_t cam_set_speed_confidence(double meter_per_sec)
{
    /*
	 * According to ETSI TS 102 894-2 V1.2.1 (2014-09)
	 * The value shall be set to:
	 * 	1 if the speed accuracy is equal to or less than 1 cm/s.
	 * 	n (n > 1 and n < 125) if the speed accuracy is equal to or less than n cm/s.
	 * 	125 if the speed accuracy is equal to or less than 125 cm/s.
	 * 	126 if the speed accuracy is out of range, i.e. greater than 125 cm/s.
	 * 	127 if the speed accuracy information is not available.
	 */

    double cm_per_sec;
    int32_t value;

    cm_per_sec = meter_per_sec * 100.0;

    if (cm_per_sec <= 1.0) {
        value = 1;
    }
    else if (cm_per_sec > 1.0 && cm_per_sec < 125.0) {
        value = (int32_t)(cm_per_sec);
    }
    else if (cm_per_sec >= 125.0 && cm_per_sec < 126.0) {
        value = 125;
    }
    else {
        value = 126;
    }

    return value;
}

/**
 *
 * @fn load_station_config(station_config_t *config)
 * @brief   load station config from file
 * @param   [out] config configuration read form file
 *
 * @return  [int]   Function executing result.
 * @retval  [0]     Success.
 * @retval  [-1]    Fail.
 */
static int load_station_config(station_config_t *config)
{
    char *content = json_fread(STATION_CONFIG_FILE_NAME);
    if (content == NULL) {
        /* File read failed, create the default config file */
        printf("The station config file not exist, create the default config file!\n");
        json_fprintf(STATION_CONFIG_FILE_NAME,
                     "{ \
                        SendCAM : [   \
                        { \
                            stationID : 168, \
                            exteriorLights : \
                            { \
                                leftTurnSignalOn : 0, \
                                rightTurnSignalOn : 0, \
                            }, \
                            role : emergency, \
                            emergency :  \
                            { \
                                lightBarInUse : 0, \
                                sirenInUse : 1, \
                                causeCode : -1, \
                            } \
                        } \
                        ] \
                    }");
        json_prettify_file(STATION_CONFIG_FILE_NAME);  // Optional
        content = json_fread(STATION_CONFIG_FILE_NAME);
    }

    if (content != NULL) {
        /* Extract setting form JSON format */
        struct json_token t_root;
        int i, len = strlen(content);

        for (i = 0; json_scanf_array_elem(content, len, ".SendCAM", i, &t_root) > 0; i++) {
            char *role = NULL;

            // EVAL COMMENT:
            //printf("Index %d, token %.*s\n", i, t_root.len, t_root.ptr);
            json_scanf(t_root.ptr, t_root.len, "{stationID: %d}", &(config->stationID));
            json_scanf(t_root.ptr, t_root.len, "{exteriorLights: {leftTurnSignalOn: %d, rightTurnSignalOn: %d}}", &(config->leftTurnSignalOn), &(config->rightTurnSignalOn));
            json_scanf(t_root.ptr, t_root.len, "{role: %Q}", &role);

            if (0 == strcmp("emergency", role)) {
                config->role = VehicleRole_emergency;
                json_scanf(t_root.ptr, t_root.len, "{emergency: {lightBarInUse: %d, sirenInUse: %d, causeCode: %d}}", &(config->lightBarInUse), &(config->sirenInUse), &(config->causeCode));
            }
            else {
                config->role = VehicleRole_Default;
            }
            if (role != NULL) {
                free(role);
            }
        }
        return 0;
    }
    else {
        printf("Load station config file failed!\n");
        return -1;
    }

    return 0;
}

static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority)
{
    int32_t result;
    itsg5_app_thread_config_t limited_thread_config;

#ifdef __SET_PRIORITY__
    struct sched_param param;
#endif  // __SET_PRIORITY__
    if (p_name == NULL) {
        return -1;
    }

    /* Check thread priority is in the limited range */
    itsg5_get_app_thread_config(&limited_thread_config);

    if (APP_THREAD_TX == type) {
        /* Check the limited range for tx thread priority */
        if ((priority < limited_thread_config.tx_thread_priority_low) || (priority > limited_thread_config.tx_thread_priority_high)) {
            /* Thread priority is out of range */
            printf("The tx thread priority is out of range (%d-%d): %d \n", limited_thread_config.tx_thread_priority_low, limited_thread_config.tx_thread_priority_high, priority);
            return -1;
        }
    }
    else if (APP_THREAD_RX == type) {
        /* Check the limited range for rx thread priority */
        if ((priority < limited_thread_config.rx_thread_priority_low) || (priority > limited_thread_config.rx_thread_priority_high)) {
            /* Thread priority is out of range */
            printf("The rx thread priority is out of range (%d-%d): %d \n", limited_thread_config.rx_thread_priority_low, limited_thread_config.rx_thread_priority_high, priority);
            return -1;
        }
    }
    else {
        /* Target thread type is unknown */
        printf("The thread type is unknown: %d \n", type);
        return -1;
    }

    result = pthread_setname_np(thread, p_name);
    if (result != 0) {
        printf("Can't set thread name: %d (%s) \n", result, strerror(result));
        return -1;
    }

#ifdef __SET_PRIORITY__
    param.sched_priority = priority;
    result = pthread_setschedparam(thread, SCHED_FIFO, &param);
    if (result != 0) {
        printf("Can't set thread priority: %d (%s) \n", result, strerror(result));
        return -1;
    }
#endif  // __SET_PRIORITY__
    return 0;
}

const char* getCurrentTime(){

    char currentTimeBuffer[1024];
    char *current;
    time_t now;
    time(&now); // Current Date and Time
    ctime(&now); // Convert to human-readable format

    struct tm *local = localtime(&now); // Store information in a struct

    snprintf(currentTimeBuffer, sizeof(currentTimeBuffer), "%d-%02d-%02d_%02d:%02d:%02d", local->tm_year + 1900, (local->tm_mon+1),local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    
    current = currentTimeBuffer;
    return current;
}

static long long getTimestamp(){
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}
