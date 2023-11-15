import asyncio
import logging
import os

from createCAM import generate_cam
from mqtt.mqtt_client import MQTTClient
from security_protocols.common import SecurityProtocolEnum
from security_protocols.protocol_setup import ProtocolSetup

logging.basicConfig(
    # Debug levels: CRITICAL, ERROR, WARNING, INFO, DEBUG, NOTSET - https://docs.python.org/3/library/logging.html
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(filename)s:%(lineno)d - %(message)s'
)

# ---------------------------- #
# -- Connect to MQTT Broker -- #
# ---------------------------- #
mqtt_client = MQTTClient()
mqtt_connection = mqtt_client.connect()
mqtt_connection.loop_start()


def send_message(sec_prot):
    # Generate message
    logging.debug(f"Generating message.")
    msg = generate_cam(static=True)

    # Convert to bytes
    msg = bytes.fromhex(msg)

    # Apply security protocol
    logging.debug(f"Applying security protocol.")
    secure_msg = sec_prot.apply_security_protocol_encapsulation(
        its_message=msg
    )

    # Send Via MQTT
    logging.debug(f"Sending generated message via MQTT")
    mqtt_client.dispatch_to_mqtt(message=secure_msg, message_type='CAM')


async def send_message_loop(sec_prot, station_id, freq):
    while True:
        logging.info(f"Station {station_id} sending message")
        send_message(sec_prot=sec_prot)
        await asyncio.sleep(delay=1 / freq)  # Sleep for y milliseconds


async def send_all_messages_1sec(sec_prot, station_id, freq):
    if freq < 1:
        raise Exception("Frequency < 1Hz while testing all messages in one second.")

    if type(freq) != int:
        raise Exception("Frequency is not an integer while testing all messages in one second..")

    ct = freq
    while ct > 0:
        ct -= 1

        logging.info(f"Station {station_id} sending message")
        send_message(sec_prot=sec_prot)
        await asyncio.sleep(delay=1 / freq)  # Sleep for y milliseconds


async def main():
    logging.debug('Message generator being initialised...')

    try:
        mode = os.getenv("ITSSEC_MSG_GEN_MODE")                                 # Specify execution mode
        security = os.getenv("ITSSEC_MSG_GEN_SECURITY")                         # Specify security mode
        num_of_its_stations = int(os.getenv("ITSSEC_MSG_GEN_N_ITS_STATIONS"))   # Number of ITS-S within range
        message_frequency = float(os.getenv("ITSSEC_MSG_GEN_FREQ"))               # Message frequency

        logging.info(f"Required environment vars: "
                     f"\nITSSEC_MSG_GEN_MODE : {mode}"
                     f"\nITSSEC_MSG_GEN_SECURITY : {security}"
                     f"\nITSSEC_MSG_GEN_N_ITS_STATIONS : {num_of_its_stations}"
                     f"\nITSSEC_MSG_GEN_FREQ : {message_frequency}")

        # Reading and verifying environment variables
        if (
                mode is None or
                security is None or
                num_of_its_stations is None or
                message_frequency is None
        ):
            logging.error("System will exit. Configure all the required environment variables.")
            exit(-1)

        # Create algorithm instance
        if security == "NoSecurity":
            security_protocol_enum = SecurityProtocolEnum.NONE
        elif security == "DLAPP":
            security_protocol_enum = SecurityProtocolEnum.DLAPP
        elif security == "MFSPV":
            security_protocol_enum = SecurityProtocolEnum.MFSPV
        else:
            logging.error(f"Unexpected security specification : {security}")
            raise Exception(f"Unexpected security specification : {security}")

        security_protocol_obj = ProtocolSetup.get_security_protocol(security_protocol_enum)

        logging.info(f"Configuration. "
                     f"Execution mode : {mode} | "
                     f"Security : {security} | "
                     f"Number of ITS Stations being simulated : {num_of_its_stations}"
                     f"Message frequency : {message_frequency}")

        if mode == "loop":

            tasks = [
                send_message_loop(
                    sec_prot=security_protocol_obj,
                    station_id=i,
                    freq=message_frequency) for i in range(num_of_its_stations)]
            await asyncio.gather(*tasks)
            mqtt_connection.loop_stop()

        elif mode == "1sec_eval":

            tasks = [
                send_all_messages_1sec(
                    sec_prot=security_protocol_obj,
                    station_id=i,
                    freq=int(message_frequency)) for i in range(num_of_its_stations)]
            await asyncio.gather(*tasks)
            mqtt_connection.loop_stop()

        else:
            logging.error(f"Unexpected execution mode: {mode}")
            raise Exception(f"Unexpected execution mode: {mode}")

    except KeyboardInterrupt:
        mqtt_connection.loop_stop()
        logging.warning("Application terminated.")
        exit()

    except Exception as e:
        mqtt_connection.loop_stop()
        logging.error(f"An exception has been raised: {repr(e)}")
        raise e


asyncio.run(main())
