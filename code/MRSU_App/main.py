import asyncio
import os
import threading
import time
import logging

from common.evaluation import EvaluationEnum, Evaluator
from communication.rsu.rsu_web_socket_client import RSUWebSocketClient
from communication.mqtt.mqtt_client import MQTTClient
from message_generator.cam_gen import MessageGenerator
from security_protocols.protocol_setup import SecurityProtocolEnum, ProtocolSetup

logging.basicConfig(
    # Debug levels: CRITICAL, ERROR, WARNING, INFO, DEBUG, NOTSET - https://docs.python.org/3/library/logging.html
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(filename)s:%(lineno)d - %(message)s'
)

if __name__ == '__main__':

    logging.debug('RSU Middleware (RSU_M) running...')

    # Specify security and evaluation modes
    security = os.getenv("ITSSEC_RSU_MIDDLEWARE_SECURITY")
    evaluation = os.getenv("ITSSEC_RSU_MIDDLEWARE_EVALUATION")
    local_gen = os.getenv("ITSSEC_RSU_MIDDLEWARE_LOCAL_GEN")

    logging.info(f"Required environment vars: "
                 f"\nITSSEC_RSU_MIDDLEWARE_SECURITY : {security}"
                 f"\nITSSEC_RSU_MIDDLEWARE_EVALUATION : {evaluation}"
                 f"\nITSSEC_RSU_MIDDLEWARE_LOCAL_GEN : {local_gen}")

    # Reading and verifying environment variables
    if (
            security is None or
            evaluation is None or
            local_gen is None
    ):
        logging.error("System will exit. Configure all the required environment variables.")
        exit(-1)

    # --------------------- #
    # -- Security Object -- #
    # --------------------- #
    if security == "NoSecurity":
        security_protocol_enum = SecurityProtocolEnum.NONE
    elif security == "DLAPP":
        security_protocol_enum = SecurityProtocolEnum.DLAPP
    elif security == "MFSPV":
        security_protocol_enum = SecurityProtocolEnum.MFSPV
    else:
        logging.error(f"Unexpected security specification : {security}")
        raise Exception(f"Unexpected security specification : {security}")

    # --------------------- #
    # -- Evaluation Mode -- #
    # --------------------- #
    if evaluation == "None":
        evaluation_enum = EvaluationEnum.NONE
    elif evaluation == "TotalComputationTime":
        evaluation_enum = EvaluationEnum.TotalComputationTime
    elif evaluation == "SecurityComputationTime":
        evaluation_enum = EvaluationEnum.SecurityComputationTime
    elif evaluation == "NetworkDelayReceiver":
        evaluation_enum = EvaluationEnum.NetworkDelayReceiver
    elif evaluation == "TransmissionToMobileNetworkDelay":
        evaluation_enum = EvaluationEnum.TransmissionToMobileNetworkDelay
    elif evaluation == "TransmissionToG5NetworkDelay":
        evaluation_enum = EvaluationEnum.TransmissionToG5NetworkDelay
    elif evaluation == "TransmissionToXferViaWSS":
        evaluation_enum = EvaluationEnum.TransmissionToXferViaWSS
    else:
        logging.error(f"Unexpected evaluation mode : {evaluation}")
        raise Exception(f"Unexpected evaluation mode : {evaluation}")

    logging.info(f'Application configuration:\n'
                 f'security = {security}\n'
                 f'evaluation = {evaluation}')

    mqtt_connection = None
    security_protocol_obj = None
    evaluator = Evaluator(evaluation_mode=evaluation_enum)

    try:
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        security_protocol_obj = ProtocolSetup.get_security_protocol(security_protocol_enum)
        logging.debug("Security protocol object obtained.")

        # ---------------------------- #
        # ------ Connect to RSU ------ #
        # ---------------------------- #
        ws_client = RSUWebSocketClient(its_security_protocol=security_protocol_obj, evaluator=evaluator)
        ws_connection = loop.run_until_complete(ws_client.connect())  # run_until_complete since it is an async function

        # Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if evaluation_enum == EvaluationEnum.TransmissionToXferViaWSS:
            def loop_wss_message():
                loop_temp = asyncio.new_event_loop()
                asyncio.set_event_loop(loop_temp)

                while True:

                    loop_temp.run_until_complete(ws_client.send_message(
                        '''{"echo":{"msg-etsi":[{"payload":"0202000003e803e8405997d7ed8cbb585e401401401430d5400000000000003e713a800bffe5fff800005a650061d298ce1c11f64e798ce257c261f458ce23f7c8daa58ce211697b1ad8ce1abdf828998ce1854a6ce5d8ce24bc5874098ce2210784fad8ce2221a877f18ce1f9956e0918ce00","gn":{"scf":false,"offload":false,"lifetime":10000,
            "transport":"SHB","chan":"CCH","tc":0},"encoding":"UPER","btp":{"type":"CAM"}}],"token":"token1"},"token":"echo_test_token"}'''
                    ))

                    time.sleep(1)

            thread = threading.Thread(target=loop_wss_message)
            thread.start()

            # If the following processing tasks reaches has been completed, the program ends
            loop.run_until_complete(asyncio.wait(
                [ws_client.receive_message(), ws_client.heartbeat()]
            ))
        # ---------------- ---------------- ---------------- ---------------- ----------------

        loop.run_until_complete(ws_client.send_message(
            ws_client.rsu_subscription  # Subscribe to interface XFER (present in P_RSU)
        ))

        # ---------------------------- #
        # -- Connect to MQTT Broker -- #
        # ---------------------------- #
        mqtt_client = MQTTClient(its_security_protocol=security_protocol_obj, evaluator=evaluator)
        mqtt_connection = mqtt_client.connect()
        mqtt_connection.loop_start()

        # Giving to MQTT's and RSU_WS's clients to each other
        ws_client.mqtt_dispatcher = mqtt_client
        mqtt_client.rsu_dispatcher = ws_client

        time.sleep(3)  # Wait for all connections
        logging.info("Initialisation tasks have been completed. Waiting for messages...\n\n")

        # ---------------------------- #
        # - Local Message Generation - #
        # ---------------------------- #
        if local_gen == "True":
            logging.info("Initialising message generator")
            message_gen = MessageGenerator(
                mqtt_dispatcher=mqtt_client,
                rsu_dispatcher=ws_client,
                sec_prot=security_protocol_obj,
                evaluator=evaluator
            )

            thread = threading.Thread(target=message_gen.message_loop)

            # Start the thread
            thread.start()

        # If the following processing tasks reaches has been completed, the program terminates
        loop.run_until_complete(asyncio.wait(
            [ws_client.receive_message(), ws_client.heartbeat()]
        ))
        mqtt_connection.loop_stop()

    except KeyboardInterrupt:
        mqtt_connection.loop_stop()
        logging.warning("Application terminated.")
        exit()

    except Exception as e:
        logging.error(f"An exception has been raised: {repr(e)}")
        mqtt_connection.loop_stop()
        raise e
