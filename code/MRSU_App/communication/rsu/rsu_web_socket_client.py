import binascii
import logging
import time

import websockets
import ssl
import threading
import asyncio
import json

from common.evaluation import EvaluationEnum
from common.its_messages_util import get_station_id, get_secure_message_from_cam
from security_protocols.common import SecurityProtocolEnum


class RSUWebSocketClient:
    """
    Web Socket Client to perform P_RSU's Xfer-related operations
    """

    def __init__(self, its_security_protocol, evaluator):

        # WebSocket Secure connection path (Siemens RSU)
        self.rsu_ws_path = "wss://10.64.10.20:443/xfer"

        # Import Certificates
        self.ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.ssl_context.load_cert_chain("communication\\rsu\\certs\\xfer-pub.crt",
                                         "communication\\rsu\\certs\\xfer.key")
        self.ssl_context.check_hostname = False
        self.ssl_context.verify_mode = ssl.CERT_NONE

        # WebSockets connection
        self.ws_connection = None

        # Security Protocol Object
        self.its_security_protocol = its_security_protocol

        # MQTT Dispatcher Object
        self.mqtt_dispatcher = None

        # Specify RSU's Xfer interface subscription:
        self.rsu_subscription = '''{"subscribe":{"reg-etsi":[{"msg":"ALL"},{"loop":"ALL"}]}}'''
        # '''{"subscribe":{"reg-etsi":[{"loop":"ALL"}]}}''' / '''{"subscribe":{"reg-etsi":[{"msg":"ALL"}]}}'''

        # Evaluation mode
        self.evaluator = evaluator

        logging.info(f"WebSocketClient Object initialised with the following info:\n"
                     f"rsu_subscription: {self.rsu_subscription}\n"
                     f"rsu_ws_path: {self.rsu_ws_path}"
                     )

    async def connect(self):
        """
        Connecting to rsu (via interface XFER)
        """
        try:
            logging.info("Connecting to RSU (via interface XFER) ...")
            ws_conn = await websockets.connect(self.rsu_ws_path, ssl=self.ssl_context)
            logging.info("Connected to RSU!")

            self.ws_connection = ws_conn
            return ws_conn

        except Exception as e:
            logging.error(f"connect function: Exception while connecting via WebSockets to rsu: {repr(e)}")

    async def send_message(self, message):
        """
        Sending a message to rsu
        """
        try:
            logging.debug(f"Sending a message to rsu: {message}")

            # Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if self.evaluator.evaluation_mode == EvaluationEnum.TransmissionToXferViaWSS:
                self.evaluator.network_delay_transmitter_start = time.perf_counter_ns()
            # ---------------- ---------------- ---------------- ---------------- ----------------

            await self.ws_connection.send(message)
        except Exception as e:
            logging.error(f"send_message function: Exception while sending a message via WebSockets to rsu: {repr(e)}")
            return

    async def receive_message(self):
        """
        Receiving all P_RSU messages and handling them
        """
        while True:

            try:
                message = await self.ws_connection.recv()

                # Evaluation zone: ---------------- ---------------- ---------------- ----------------
                if self.evaluator.evaluation_mode == EvaluationEnum.TransmissionToG5NetworkDelay and \
                        "bc001098c3736503" in message:  # OBU addr
                    self.evaluator.network_delay_transmitter_end = time.perf_counter_ns()
                    self.evaluator.save_transmission_delay(
                        sec_prot_designation=self.its_security_protocol.protocol_designation)
                    continue
                # ---------------- ---------------- ---------------- ---------------- ----------------

                # Evaluation zone: ---------------- ---------------- ---------------- ----------------
                if self.evaluator.evaluation_mode == EvaluationEnum.TransmissionToXferViaWSS and \
                        "echo_test_token" in message:
                    self.evaluator.network_delay_transmitter_end = time.perf_counter_ns()
                    self.evaluator.save_transmission_delay(
                        sec_prot_designation="NONE")
                    continue
                # ---------------- ---------------- ---------------- ---------------- ----------------

                logging.info("***** MESSAGE RECEIVED FROM RSU *****")

                threading.Thread(target=self.handle_message, args=(message,)).start()

            except Exception as e:
                logging.error("receive_message function: Exception while receiving a message via WebSockets from rsu: ",
                              repr(e))
                return

    def handle_message(self, message):
        """
        Handle messages received by the P_RSU
        """

        try:

            # Convert message in JSON
            msg_json = json.loads(message)

            if 'success' in msg_json:
                # RSU success reply
                logging.debug(f"RSU returned a success message: {msg_json['success']}")
                return
            elif 'error' in msg_json:
                # RSU error reply
                logging.error(f"RSU returned an error message: {msg_json['error']}")
                return

            # Handle Message
            if 'msg-etsi' in msg_json:
                payload = msg_json['msg-etsi'][0]["payload"]

            elif 'echo' in msg_json:
                payload = msg_json["echo"]
                logging.debug(f"Echo payload : {payload}")
                return

            else:
                return

            # Station ID
            station_id = get_station_id(its_message=binascii.unhexlify(payload),
                                        msg_type=msg_json['msg-etsi'][0]['btp']['type'])
            logging.debug(f"Station id: {station_id}")

            # ORIGIN: Initially sent via MQTT
            # - A smartphone, i.e., mobile app -> mqtt -> rsu middleware -> physical rsu -> rsu middleware
            # (due to subscription for all Downstream messages:)
            if station_id == 200:
                logging.info("Message origin: Its an echo message (initially sent via MQTT), ignoring.")
                return

            if station_id == 1000:
                logging.info("Message origin: Its an echo message (initially sent via LocalGen), ignoring.")
                return

            self.handle_its_message(msg_json=msg_json, msg_payload=payload)

            # Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if self.evaluator.evaluation_mode == EvaluationEnum.TotalComputationTime:
                self.evaluator.execution_performance(
                    execution_lambda=lambda: self.handle_its_message(msg_json=msg_json, msg_payload=payload),
                    identifier=f'FromG5ToMQTT_'
                               f'{SecurityProtocolEnum[self.its_security_protocol.protocol_designation.name]}'
                               f'[TimesInMs]',
                )
            # ---------------- ---------------- ---------------- ---------------- ----------------

        except Exception as e:
            logging.error(f"handle_message function: Exception while handling rsu's message: {repr(e)}")
            raise e

    def handle_its_message(self, msg_json, msg_payload):
        """
        ITS messages' handler (received by the P_RSU)
        """

        # At this point is either a message created by:
        # The RSU itself or by an OBU or other ITS-S (which have sent to the rsu)

        logging.debug(f"Message json: {json.dumps(msg_json)}")

        # -------- Obtaining necessary info:
        msg_payload_bytes = bytes.fromhex(msg_payload)

        # Get message type (CAM or DENM)
        msg_type = msg_json['msg-etsi'][0]['btp']['type']

        # Station ID
        station_id = get_station_id(its_message=binascii.unhexlify(msg_payload), msg_type=msg_type)
        logging.debug(f"Station id: {station_id}")

        # At this point we need to check whether the message has as source:
        # - The RSU itself
        # - Other ITS stations (OBUs and RSUs) that have sent the message to the RSU via ITS-G5

        # ORIGIN: An ITS-Station sent a message to RSU via G5
        if station_id == 168:

            logging.info("Message origin: An ITS-Station (OBU in this case) sent a message to RSU via G5")
            logging.info("Proceeding to Security verification.")

            # 1. Get security fields and transform message to what supposed to receive
            # (sec fields concatenated with message not inside)
            secure_msg = get_secure_message_from_cam(its_message=msg_payload_bytes, sec_prot=self.its_security_protocol)

            # 2. Use the previous result to Decapsulation / Verification
            msg_from_decapsulation = self.its_security_protocol.apply_security_protocol_decapsulation(encapsulated_its_message=secure_msg)

            if msg_from_decapsulation is None:
                logging.info("Message not successfully verified, not sending to RSU.")
                return

            # Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if self.evaluator.evaluation_mode == EvaluationEnum.SecurityComputationTime:
                self.evaluator.execution_performance(
                    execution_lambda=lambda: self.its_security_protocol.apply_security_protocol_decapsulation(
                        encapsulated_its_message=secure_msg
                    ),
                    identifier=f'MessageDecapsulationReceivedFromG5_'
                               f'{SecurityProtocolEnum[self.its_security_protocol.protocol_designation.name]}'
                               f'[TimesInMs]',
                )
            # ---------------- ---------------- ---------------- ---------------- ----------------

        # ORIGIN: RSU HOST
        elif station_id == 1195776:

            logging.info("Message origin: RSU Host")

            logging.info("Encapsulating message with the security protocol.")
            logging.debug(f"Message before encapsulation: {msg_payload_bytes}")

            secure_msg = self.its_security_protocol.apply_security_protocol_encapsulation(
                its_message=msg_payload_bytes
            )

            logging.info("Security protocol encapsulation completed.")
            logging.debug(f"Message after encapsulation: {secure_msg}")

        else:
            logging.error(f"Unexpected station id: {station_id}")
            raise Exception(f"Unexpected station id: {station_id}")

        logging.debug(f"Dispatching message so the MQTT client can publish it. Message: {secure_msg}")
        self.mqtt_dispatcher.dispatch_to_mqtt(message=secure_msg, message_type=msg_type)

    def dispatch_to_rsu(self, message, message_type):
        """
        Dispatch do P_RSU via XFER

        @param message: Message to dispatch
        @param message_type: e.g., 'CAM'
        """

        logging.debug(f"dispatch_to_rsu called.")
        logging.debug(f"Message being dispatched: {message}")

        # Refer to RSU's Xfer interface documentation for more detail
        if message_type == 'CAM':
            msg = '''{"msg-etsi":[{"payload":"%s","gn":{"scf":false,"offload":false,"lifetime":10000,
            "transport":"SHB","chan":"CCH","tc":0},"encoding":"UPER","btp":{"type":"CAM"}}],"token":"token1"}''' % \
                  message

        elif message_type == 'DENM':
            msg = '''{"msg-etsi":[{"payload":"%s","gn":{"scf":false,"offload":false,"lifetime":30000,
            "transport":"GBC","chan":"CCH","tc":0},"encoding":"UPER","btp":{"type":"DENM"}}],"token":"token2"}''' % \
                  message

        else:
            logging.error(f"Unexpected message type: {message_type}")
            raise Exception(f"Unexpected message type: {message_type}")

        temp_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(temp_loop)

        # Send message to rsu via Xfer interface
        temp_loop.run_until_complete(self.send_message(msg))

        logging.info("Message sent to PhysicalRSU via Xfer.")

    async def heartbeat(self):
        """
        Sending heartbeat to server every 25 seconds
        Ping - pong messages to verify connection is alive
        """

        while True:

            try:
                await self.ws_connection.send('''{"echo":"Ping"}''')
                await asyncio.sleep(25)
            except Exception as e:
                logging.error(f"heartbeat function: Exception while sending heartbeats to rsu: {repr(e)}")
                return
