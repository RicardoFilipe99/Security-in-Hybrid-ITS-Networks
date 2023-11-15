import binascii
import logging
import time

import paho.mqtt.client as mqtt
import random

from common.evaluation import EvaluationEnum
from common.its_messages_util import include_security_fields_in_cam
from security_protocols.common import SecurityProtocolEnum


class MQTTClient:
    """
    MQTT Client to perform MQTT-related operations
    """

    def __init__(self, its_security_protocol, evaluator):

        # MQTT Client Object
        self.mqtt_client = None

        # Security Protocol Object
        self.its_security_protocol = its_security_protocol

        # RSU WebSocket Client (Dispatcher from the MQTT's POV)
        self.rsu_dispatcher = None

        # MQTT Broker ip and port
        self.broker_ip = '192.68.221.36'
        self.broker_port = 1883

        # Generating and logging MQTT Client ID
        self.my_mqtt_client_id = f'RSUMiddleware-{random.randint(0, 10000)}'

        # Evaluation mode
        self.evaluator = evaluator

        # From the RSUs PoV, they only are interested in smartphone's CAMs and DENMs, not other RSU or OBUs, as
        # they should communicate via ITS-G5

        # Subscribing topics
        self.topic_prefix_for_sub = "its/security/hybrid-network/mobile-app/upstream"
        self.topic_for_sub = self.topic_prefix_for_sub + '/#'

        # Publishing topics
        self.topic_cam_prefix_for_pub = "its/security/hybrid-network/mobile-app/downstream/cam"
        self.my_cam_topic_to_publish = f'{self.topic_cam_prefix_for_pub}/{self.my_mqtt_client_id}'

        self.topic_denm_prefix_for_pub = "its/security/hybrid-network/mobile-app/downstream/denm"
        self.my_denm_topic_to_publish = f'{self.topic_denm_prefix_for_pub}/{self.my_mqtt_client_id}'

        logging.info(f"MQTT Client Object initialised with the following info:\n"
                     f"broker: {self.broker_ip}\n"
                     f"port: {self.broker_port}\n"
                     f"client_id: {self.my_mqtt_client_id}\n"
                     f"topic_for_sub: {self.topic_for_sub}\n"
                     f"my_cam_topic_to_publish: {self.my_cam_topic_to_publish}\n"
                     f"my_denm_topic_to_publish: {self.my_denm_topic_to_publish}"
                     )

    def connect(self):
        """
        Connects to MQTT Broker
        """

        logging.info("Connecting to MQTT Broker ...")

        # Creating MQTT Client
        client = mqtt.Client(self.my_mqtt_client_id)
        client.on_connect = self.on_connect
        client.on_message = self.on_message
        client.on_disconnect = self.on_disconnect

        # Connecting to MQTT broker and subscribing to CAM and DENM topics
        client.connect(self.broker_ip, self.broker_port)
        client.subscribe(self.topic_for_sub)

        self.mqtt_client = client
        return self.mqtt_client

    def on_connect(self, client, userdata, flags, rc):
        """
        Callback when the broker responds to our connection request (used on 'connect')

        The value of rc indicates success or not:
            0: Connection successful
            1: Connection refused - incorrect protocol version
            2: Connection refused - invalid client identifier
            3: Connection refused - server unavailable
            4: Connection refused - bad username or password
            5: Connection refused - not authorised
        """

        if rc == 0:
            logging.info("Connected to MQTT Broker!")
        else:
            logging.error(f"Failed to connect to MQTT Broker, return code {rc}\n")

    def on_disconnect(self, client, userdata, rc=0):
        """
        Callback when disconnected to the broker (used on 'connect')
        """

        logging.error("MQTT on disconnect called.")
        client.loop_stop()

    def on_message(self, client, userdata, message):
        """
        Callback when a message arrives (used on 'connect')
        """

        # Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if self.evaluator.evaluation_mode == EvaluationEnum.TransmissionToMobileNetworkDelay:
            self.evaluator.network_delay_transmitter_end = time.perf_counter_ns()
            self.evaluator.save_transmission_delay(sec_prot_designation=self.its_security_protocol.protocol_designation)
            return
        # ---------------- ---------------- ---------------- ---------------- ----------------

        # Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if self.evaluator.evaluation_mode == EvaluationEnum.NetworkDelayReceiver:
            self.dispatch_to_mqtt(message=message.payload, message_type='CAM')
        # ---------------- ---------------- ---------------- ---------------- ----------------

        # Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if self.evaluator.evaluation_mode == EvaluationEnum.TotalComputationTime:
            self.evaluator.execution_performance(
                execution_lambda=lambda: self.handle_message(message),
                identifier=f'FromMQTTToG5_'
                           f'{SecurityProtocolEnum[self.its_security_protocol.protocol_designation.name]}'
                           f'[TimesInMs]',
            )
        # ---------------- ---------------- ---------------- ---------------- ----------------

        self.handle_message(message)

    def handle_message(self, message):
        """
        Message handler. If the message is successfully verified it is dispatched to P_RSU
        """

        logging.info("***** MESSAGE RECEIVED FROM MQTT *****")

        msg_payload = message.payload
        received_msg_payload = message.payload
        topic = message.topic

        logging.info(f"Topic: {topic} | Message payload : {msg_payload}")

        if 'cam' in topic:
            message_type = 'CAM'
        elif 'denm' in topic:
            message_type = 'DENM'
        else:
            logging.error(f"Unexpected message type from topic: {topic}")
            raise Exception(f"Unexpected message type from topic: {topic}")

        logging.debug(f"MQTT received message. MessageType: {message_type} Topic: {topic} | Payload: {msg_payload}")
        logging.debug("About to verify and do protocol decapsulation.")

        msg_payload = self.its_security_protocol.apply_security_protocol_decapsulation(
            encapsulated_its_message=received_msg_payload)

        # If msg_payload is None the message was not correctly verified
        if msg_payload is None:
            logging.info("Message not successfully verified, not sending to RSU.")
            return

        # Evaluation zone: ---------------- ---------------- ---------------- ----------------
        if self.evaluator.evaluation_mode == EvaluationEnum.SecurityComputationTime:
            self.evaluator.execution_performance(
                execution_lambda=lambda: self.its_security_protocol.apply_security_protocol_decapsulation(
                    encapsulated_its_message=received_msg_payload),
                identifier=f'MessageDecapsulationReceivedFromMQTT_'
                           f'{SecurityProtocolEnum[self.its_security_protocol.protocol_designation.name]}'
                           f'[TimesInMs]',
            )
        # ---------------- ---------------- ---------------- ---------------- ----------------

        logging.info("Calling RSU dispatcher.")

        # Insert security fields in CAMs
        if message_type == 'CAM':
            security_fields_bytes = self.its_security_protocol.get_security_fields_bytes(
                encapsulated_its_message=received_msg_payload)

            msg_payload = include_security_fields_in_cam(its_message=msg_payload,
                                                         security_fields_bytes=security_fields_bytes)
        elif message_type == 'DENM':
            raise NotImplemented
        else:
            raise Exception(f"Unknown topic: {topic}")

        msg = binascii.hexlify(msg_payload).decode()
        self.rsu_dispatcher.dispatch_to_rsu(msg, message_type)

    def dispatch_to_mqtt(self, message, message_type):
        """
        Dispatch messages to broker
        """

        logging.debug("dispatch_to_mqtt called, publishing message.")

        if message_type == 'CAM':
            self.mqtt_client.publish(self.my_cam_topic_to_publish, message, qos=0)  # As CAM is a periodic status
            # message, QoS 0 can be a reasonable choice

            # Evaluation zone: ---------------- ---------------- ---------------- ----------------
            if self.evaluator.evaluation_mode == EvaluationEnum.TransmissionToMobileNetworkDelay:
                self.evaluator.network_delay_transmitter_start = time.perf_counter_ns()
            # ---------------- ---------------- ---------------- ---------------- ----------------

        elif message_type == 'DENM':
            self.mqtt_client.publish(self.my_denm_topic_to_publish, message, qos=1)

        logging.info("Message published via MQTT")
