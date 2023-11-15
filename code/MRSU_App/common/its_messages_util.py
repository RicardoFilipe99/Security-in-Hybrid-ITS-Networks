import binascii
import logging
import math
import asn1tools

from common.util import create_signed_byte_array_from

cam_its = asn1tools.compile_files(['common/asn_files/its.asn', 'common/asn_files/cam.asn'], 'uper')
denm_its = asn1tools.compile_files(['common/asn_files/its.asn', 'common/asn_files/denm.asn'], 'uper')


def include_security_fields_in_cam(its_message, security_fields_bytes):
    """
    Inserts the 'security_fields_bytes' into the 'its_message' by using the PathHistory field

    @param its_message: ITS message bytes
    @param security_fields_bytes: byte array with security bytes

    @return: New ITS Message but with the security bytes included in the Path History field
    """

    if security_fields_bytes is None:
        logging.info("Security fields not inserted in the ITS message (no security is being used). "
                     "Returning original CAM.")
        return its_message

    logging.debug(f'include_security_fields_in_cam called.\n'
                  f'its_message len: {len(its_message)} | '
                  f'its_message: {its_message} | '
                  f'security_fields_bytes len: {len(security_fields_bytes)} | '
                  f'security_fields_bytes: {security_fields_bytes}')

    cam_message = cam_its.decode('CAM', its_message)
    logging.debug(f"Message (CAM) decoded: {cam_message}")

    class PathPoint:
        """
        Helper class to create PathPoint objects that will include the security information
        """

        def __init__(self, DeltaLatitude, DeltaLongitude):
            self.deltaLatitude = DeltaLatitude      # 2 bytes
            self.deltaLongitude = DeltaLongitude    # 2 bytes
            self.deltaAltitude = 0

        def __str__(self):
            representation = f"PathPoint(" \
                             f"deltaLatitude={self.deltaLatitude}, " \
                             f"deltaLongitude={self.deltaLongitude}, " \
                             f"deltaAltitude={self.deltaAltitude})"

            return representation

    def split_security_fields_into_chunks(fields_bytes, chunk_size):
        """
        Helper function to split the security fields bytes into chunks
        """
        chunks = [fields_bytes[i:i + chunk_size] for i in range(0, len(fields_bytes), chunk_size)]
        return chunks

    # Split the fields into chunks of 4 bytes each (we assign 2 bytes to each fields, DeltaLatitude and DeltaLongitude)
    chunks_size = 4
    fields_chunks = split_security_fields_into_chunks(security_fields_bytes, chunks_size)
    logging.debug(f"fields_chunks: {fields_chunks}")
    logging.debug(f"fields_chunks len: {len(fields_chunks)}")

    result = len(security_fields_bytes) / chunks_size
    num_of_path_points = math.ceil(result)

    # Create a list of PathPoint objects
    path_points = []
    for _ in range(num_of_path_points):
        path_point = PathPoint(DeltaLatitude=0, DeltaLongitude=0)
        path_points.append(path_point)

    def assign_chunks_to_path_points(path_points_, sec_fields_chunks):
        """
        Helper function to assign chunks to path points objects
        """
        for i, chunk in enumerate(sec_fields_chunks):
            # Assign the first 2 bytes to DeltaLatitude and the next 2 bytes to DeltaLongitude
            setattr(path_points_[i], 'deltaLatitude', int.from_bytes(chunk[:2], 'big', signed=True))
            setattr(path_points_[i], 'deltaLongitude', int.from_bytes(chunk[2:4], 'big', signed=True))

    # Assign the fields chunks to the PathPoint objects
    assign_chunks_to_path_points(path_points_=path_points, sec_fields_chunks=fields_chunks)

    # BasicVehicleContainerLowFrequency creation
    vehicle_role = "default"  # Dont care data
    exterior_lights = (bytes([0, 0, 0, 0, 0, 0, 0, 0]), 0)  # Dont care data
    path_history = []  # Important data
    for pp in path_points:
        path_point = {
            "pathPosition": {
                "deltaLatitude": pp.deltaLatitude,
                "deltaLongitude": pp.deltaLongitude,
                "deltaAltitude": pp.deltaAltitude
            }
        }

        path_history.append(path_point)

    logging.debug(f"Resulting Path History field: {path_history}")

    basic_vehicle_container_low_frequency = {
        "vehicleRole": vehicle_role,
        "exteriorLights": exterior_lights,
        "pathHistory": path_history
    }

    low_frequency_container = ("basicVehicleContainerLowFrequency", basic_vehicle_container_low_frequency)
    cam_message['cam']['camParameters']['lowFrequencyContainer'] = low_frequency_container

    logging.debug(f"CAM after pathHistory modification: {cam_message}")

    logging.info("Security fields inserted in the ITS message, returning updated CAM.")

    return cam_its.encode("CAM", cam_message)


def get_secure_message_from_cam(its_message, sec_prot):
    """
    Gets a secure message by inspecting thE PathHistory field in a CAM message

    @param its_message: ITS message
    @param sec_prot: Security approach in use

    @return: security bytes + original its message
    """

    if sec_prot.protocol_designation.name == "NONE":
        logging.info("Security fields were not inserted in the ITS message (no security is being used). "
                     "Returning original CAM.")
        return its_message

    logging.debug(f'include_security_fields_in_cam called.\n'
                  f'its_message len: {len(its_message)} | '
                  f'its_message: {its_message}')

    cam_message = cam_its.decode('CAM', its_message)
    logging.debug(f"Message (CAM) decoded: {cam_message}")

    # Next step: Extract path points and get all bytes from path points numbers and concatenate them
    r_decoded_cam = cam_its.decode("CAM", its_message)
    r_path_hist = r_decoded_cam['cam']['camParameters']['lowFrequencyContainer'][1]['pathHistory']
    logging.debug(f"r_path_hist: {r_path_hist}")

    # Access the fields in the PathPoint objects to retrieve the security fields
    retrieved_fields = b''.join([
        path_point['pathPosition']['deltaLatitude'].to_bytes(2, 'big', signed=False) +
        path_point['pathPosition']['deltaLongitude'].to_bytes(2, 'big', signed=False)
        for path_point in r_path_hist
    ])

    logging.debug(f"retrieved_fields: {create_signed_byte_array_from(retrieved_fields)}")

    # Remove low frequency from message to get original message
    del r_decoded_cam['cam']['camParameters']['lowFrequencyContainer']
    logging.debug(f"CAM after removing low frequency: {r_decoded_cam}")

    # Concatenate and return: Sec fields + Original message
    updated_cam = cam_its.encode('CAM', r_decoded_cam)
    logging.debug(f"CAM updated hex: {binascii.hexlify(updated_cam).decode('ascii')}")

    logging.info("Security fields extracted. Returning concatenated secure CAM.")

    return retrieved_fields + updated_cam


def get_station_id(its_message, msg_type):
    """
    Extracts the message's station id

    @param its_message: ITS message
    @param msg_type: Type, e.g., 'CAM' or 'DENM'

    @return: Station id value
    """

    if msg_type == 'CAM':
        cam_message = cam_its.decode('CAM', its_message)
        return cam_message['header']['stationID']
    elif msg_type == 'DENM':
        denm_message = denm_its.decode('DENM', its_message)
        return denm_message['header']['stationID']
