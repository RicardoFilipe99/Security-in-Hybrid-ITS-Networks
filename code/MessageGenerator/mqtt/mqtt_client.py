import logging
import paho.mqtt.client as mqtt
import random


class MQTTClient:
    """
    MQTT Client to perform MQTT-related operations
    """

    def __init__(self):

        # MQTT Client Object
        self.mqtt_client = None

        # MQTT Broker ip and port
        self.broker_ip = '192.68.221.36'
        self.broker_port = 1883

        # Generating and logging MQTT Client ID
        self.my_mqtt_client_id = f'MessageGenerator-{random.randint(0, 10000)}'

        # Publishing topics
        self.topic_cam_prefix_for_pub = "its/security/hybrid-network/mobile-app/upstream/cam"
        self.my_cam_topic_to_publish = f'{self.topic_cam_prefix_for_pub}/{self.my_mqtt_client_id}'

        self.topic_denm_prefix_for_pub = "its/security/hybrid-network/mobile-app/upstream/denm"
        self.my_denm_topic_to_publish = f'{self.topic_denm_prefix_for_pub}/{self.my_mqtt_client_id}'

        logging.info(f"MQTT Client Object initialised with the following info:\n"
                     f"broker: {self.broker_ip}\n"
                     f"port: {self.broker_port}\n"
                     f"client_id: {self.my_mqtt_client_id}\n"
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

        self.mqtt_client = client
        return self.mqtt_client

    def on_connect(self, client, userdata, flags, rc):
        """
        This method is called when the broker responds to our connection request.

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

        logging.error("Error : on message called.")

    def dispatch_to_mqtt(self, message, message_type):
        """
        Dispatch messages to broker
        """
        
        logging.debug("dispatch_to_mqtt called, publishing message.")

        if message_type == 'CAM':
            self.mqtt_client.publish(self.my_cam_topic_to_publish, message, qos=0)  # As CAM is a periodic status
            # message, QoS 0 can be a reasonable choice

        elif message_type == 'DENM':
            self.mqtt_client.publish(self.my_denm_topic_to_publish, message, qos=1)

        logging.info("Message published via MQTT")
