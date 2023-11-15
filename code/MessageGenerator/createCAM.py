import binascii
import logging

import asn1tools

cam_its = asn1tools.compile_files(['files/its.asn', 'files/cam.asn'], 'uper')


def generate_cam(static):
    try:
        if static:
            return '0202000000C803E8005997D7ED8CBB585E401401401430D5400000000000003E713A800BFFE5FFF800'

        station_id = 200
        logging.info(f"generate_cam called.")

        # Basic Container
        station_type = 5

        # Basic Container - ReferencePosition
        latitude = int(387569260)
        longitude = int(-91164430)

        semi_major_confidence = 10
        semi_minor_confidence = 10
        semi_major_orientation = 10

        position_confidence_ellipse = {
            "semiMajorConfidence": semi_major_confidence,
            "semiMinorConfidence": semi_minor_confidence,
            "semiMajorOrientation": semi_major_orientation
        }

        altitude_value = 10
        altitude_confidence = "alt-000-01"

        altitude = {
            "altitudeValue": altitude_value,
            "altitudeConfidence": altitude_confidence
        }

        reference_position = {
            "latitude": latitude,
            "longitude": longitude,
            "positionConfidenceEllipse": position_confidence_ellipse,
            "altitude": altitude
        }

        basic_container = {
            "stationType": station_type,
            "referencePosition": reference_position
        }

        # BasicVehicleContainerHighFrequency
        if station_type == 15:
            # RSUContainerHighFrequency
            protected_communication_zones_rsu = {}

            rsu_container_high_frequency = {
                "protectedCommunicationZoneRSU": protected_communication_zones_rsu
            }

            high_frequency_container = ("rsuContainerHighFrequency", rsu_container_high_frequency)

        else:
            heading_value = 0
            heading_confidence = 1

            heading = {
                "headingValue": heading_value,
                "headingConfidence": heading_confidence
            }

            speed_value = 0
            speed_confidence = 1

            speed = {
                "speedValue": speed_value,
                "speedConfidence": speed_confidence
            }

            drive_direction = "forward"

            vehicle_length_value = 6000
            vehicle_length_confidence_indication = "noTrailerPresent"

            vehicle_length = {
                "vehicleLengthValue": vehicle_length_value,
                "vehicleLengthConfidenceIndication": vehicle_length_confidence_indication
            }

            vehicle_width = 40

            longitudinal_acceleration_value = 0
            longitudinal_acceleration_confidence = 1

            longitudinal_acceleration = {
                "longitudinalAccelerationValue": longitudinal_acceleration_value,
                "longitudinalAccelerationConfidence": longitudinal_acceleration_confidence
            }

            curvature_value = 0
            curvature_confidence = "unavailable"

            curvature = {
                "curvatureValue": curvature_value,
                "curvatureConfidence": curvature_confidence
            }

            curvature_calculation_mode = "yawRateNotUsed"

            yaw_rate_value = 0
            yaw_rate_confidence = "unavailable"

            yaw_rate = {
                "yawRateValue": yaw_rate_value,
                "yawRateConfidence": yaw_rate_confidence
            }

            acceleration_control = (bytes([0, 0, 0, 0, 0, 0, 0]), 7)

            lane_position = 1

            steering_wheel_angle_value = 0
            steering_wheel_angle_confidence = 1

            steering_wheel_angle = {
                "steeringWheelAngleValue": steering_wheel_angle_value,
                "steeringWheelAngleConfidence": steering_wheel_angle_confidence
            }

            lateral_acceleration_value = 0
            lateral_acceleration_confidence = 1

            lateral_acceleration = {
                "lateralAccelerationValue": lateral_acceleration_value,
                "lateralAccelerationConfidence": lateral_acceleration_confidence
            }

            vertical_acceleration_value = 0
            vertical_acceleration_confidence = 1

            vertical_acceleration = {
                "verticalAccelerationValue": vertical_acceleration_value,
                "verticalAccelerationConfidence": vertical_acceleration_confidence
            }

            performance_class = 0

            protected_zone_latitude = 0
            protected_zone_longitude = 0
            cen_dsrc_tolling_zone_id = 0

            cen_dsrc_tolling_zone = {
                "protectedZoneLatitude": protected_zone_latitude,
                "protectedZoneLongitude": protected_zone_longitude,
                "cenDsrcTollingZoneID": cen_dsrc_tolling_zone_id  # Optional
            }

            # High Frequency Container

            basic_vehicle_container_high_frequency = {
                "heading": heading,
                "speed": speed,
                "driveDirection": drive_direction,
                "vehicleLength": vehicle_length,
                "vehicleWidth": vehicle_width,
                "longitudinalAcceleration": longitudinal_acceleration,
                "curvature": curvature,
                "curvatureCalculationMode": curvature_calculation_mode,
                "yawRate": yaw_rate,
                "accelerationControl": acceleration_control,  # OPTIONAL
                "lanePosition": lane_position,  # OPTIONAL
                "steeringWheelAngle": steering_wheel_angle,  # OPTIONAL
                "lateralAcceleration": lateral_acceleration,  # OPTIONAL
                "verticalAcceleration": vertical_acceleration,  # OPTIONAL
                "performanceClass": performance_class,  # OPTIONAL
                "cenDsrcTollingZone": cen_dsrc_tolling_zone  # OPTIONAL
            }

            high_frequency_container = (
                "basicVehicleContainerHighFrequency", basic_vehicle_container_high_frequency)

        # BasicVehicleContainerLowFrequency

        vehicle_role = "default"

        exterior_lights = (bytes([0, 0, 0, 0, 0, 0, 0, 0]), 0)

        delta_latitude = 0
        delta_longitude = 0
        delta_altitude = 0

        path_position = {
            "deltaLatitude": delta_latitude,
            "deltaLongitude": delta_longitude,
            "deltaAltitude": delta_altitude
        }

        path_delta_time = 10

        path_point = {
            "pathPosition": path_position,
            "pathDeltaTime": path_delta_time
        }

        path_history = [path_point]

        basic_vehicle_container_low_frequency = {
            "vehicleRole": vehicle_role,
            "exteriorLights": exterior_lights,
            "pathHistory": path_history
        }

        low_frequency_container = ("basicVehicleContainerLowFrequency", basic_vehicle_container_low_frequency)

        # SpecialVehicleContainer

        # SpecialVehicleContainer = ("", null)
        container_flag = False

        if vehicle_role == "publicTransport":
            # PublicTransportContainer
            container_flag = True

            embarkation_status = False

            pt_activation_type = 0
            pt_activation_data = bytes([0, 0])

            pt_activation = {
                "ptActivationType": pt_activation_type,
                "ptActivationData": pt_activation_data
            }

            public_transport_container = {
                "embarkationStatus": embarkation_status,
                "ptActivation": pt_activation
            }

            special_vehicle_container = ("publicTransportContainer", public_transport_container)

        elif vehicle_role == "specialTransport":
            # SpecialTransportContainer
            container_flag = True

            special_transport_type = (bytes([0, 0, 0, 0]), 0)
            light_bar_siren_in_use = (bytes([0, 0]), 0)

            special_transport_container = {
                "specialTransportType": special_transport_type,
                "lightBarSirenInUse": light_bar_siren_in_use
            }

            special_vehicle_container = ("specialTransportContainer", special_transport_container)

        elif vehicle_role == "dangerousGoods":
            # DangerousGoodsContainers
            container_flag = True

            dangerous_goods_basic = "explosives1"

            dangerous_goods_container = {
                "dangerousGoodsBasic": dangerous_goods_basic
            }

            special_vehicle_container = ("dangerousGoodsContainer", dangerous_goods_container)

        elif vehicle_role == "roadWork":
            # RoadWorksContainerBasic
            container_flag = True

            road_works_sub_cause_code = 0
            light_bar_siren_in_use = (bytes([0, 0]), 0)

            innerhard_shoulder_status = "availableForStopping"
            outterhard_shoulder_status = "availableForStopping"
            driving_lane_status = (bytes([0, 0]), 0)

            closed_lanes = {
                "innerhardShoulderStatus": innerhard_shoulder_status,
                "outterhardShoulderStatus": outterhard_shoulder_status,
                "drivingLaneStatus": driving_lane_status
            }

            road_works_container_basic = {
                "roadWorksSubCauseCode": road_works_sub_cause_code,
                "lightBarSirenInUse": light_bar_siren_in_use,
                "closedLanes": closed_lanes
            }

            special_vehicle_container = ("roadWorksContainerBasic", road_works_container_basic)

        elif vehicle_role == "rescue":
            # RescueContainer
            container_flag = True

            light_bar_siren_in_use = (bytes([0, 0]), 0)

            rescue_container = {
                "lightBarSirenInUse": light_bar_siren_in_use
            }

            special_vehicle_container = ("rescueContainer", rescue_container)

        elif vehicle_role == "emergency":
            # EmergencyContainer
            container_flag = True

            light_bar_siren_in_use = (bytes([0, 0]), 0)
            emergency_priority = (bytes([0, 0]), 0)

            cause_code_type = 0
            sub_cause_code_type = 0

            incident_indication = {
                "causeCode": cause_code_type,
                "subCauseCode": sub_cause_code_type
            }

            emergency_container = {
                "lightBarSirenInUse": light_bar_siren_in_use,
                "incidentIndication": incident_indication,
                "emergencyPriority": emergency_priority
            }

            special_vehicle_container = ("emergencyContainer", emergency_container)

        elif vehicle_role == "safetyCar":
            # SafetyContainer
            container_flag = True

            light_bar_siren_in_use = (bytes([0, 0]), 0)

            cause_code = 0
            sub_cause_code = 0

            traffic_rule = "noPassing"
            speed_limit = 100

            incident_indication = {
                "causeCode": cause_code,
                "subCauseCode": sub_cause_code
            }

            safety_container = {
                "lightBarSirenInUse": light_bar_siren_in_use,
                "incidentIndication": incident_indication,
                "trafficRule": traffic_rule,
                "speedLimit": speed_limit
            }

            special_vehicle_container = ("safetyCarContainer", safety_container)

        if container_flag:
            cam_parameters = {
                "basicContainer": basic_container,
                "highFrequencyContainer": high_frequency_container,
                "lowFrequencyContainer": low_frequency_container,  # Optional
                "specialVehicleContainer": special_vehicle_container  # Optional
            }

        else:
            cam_parameters = {
                "basicContainer": basic_container,
                "highFrequencyContainer": high_frequency_container,
                "lowFrequencyContainer": low_frequency_container  # Optional
            }

        # ItsPduHeader
        protocol_version = 2

        message_id = 2

        # CoopAwareness
        generation_delta_time = 1000

        # CAM message fields
        its_pdu_header = {"protocolVersion": protocol_version, "messageID": message_id, "stationID": station_id}

        coop_awareness = {"generationDeltaTime": generation_delta_time, "camParameters": cam_parameters}

        # CAM
        cam = {"header": its_pdu_header, "cam": coop_awareness}

        message = cam_its.encode("CAM", cam)

        return binascii.hexlify(message).decode('ascii')

    except Exception as e:
        logging.error(f"An exception has been raised: {repr(e)}")
        raise e
