package pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher

import android.util.Log
import androidx.compose.runtime.MutableState
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.IReceiveMessageService
import kotlin.system.exitProcess


const val TAG_MQTT = "ITSMobApp-MQTT"

class MessageDispatcherMQTTImpl(
    override val receiveService : IReceiveMessageService,
    private val evaluation : Evaluator.EvaluationEnum,
) : IMessageDispatcher {

    override lateinit var vmScope: CoroutineScope
    private lateinit var mqttClient : MqttClient
    override lateinit var messagingHistory: MutableState<List<String>?>
    override lateinit var evaluator: Evaluator

    // MQTT server host
    val mqttBrokerAddress = "tcp://192.68.221.36:1883"

    // MQTT Connection Credentials
    val mqttClientIdUplinkEvents = "MobileClient-" + (0..10000).random()

    // Notice the usage of wildcards '+' and '#' (source: https://www.hivemq.com/blog/mqtt-essentials-part-5-mqtt-topics-best-practices/)

    // Topics to sub
    private val topicPrefixUpstream = "its/security/hybrid-network/mobile-app/upstream"
    private val topicPrefixDownstream = "its/security/hybrid-network/mobile-app/downstream"

    private val topicUpstreamEvents = "$topicPrefixUpstream/#"
    private val topicDownstreamEvents = "$topicPrefixDownstream/#"

    // Topics to pub
    private val myMqttTopicToPubCamEvents = "$topicPrefixUpstream/cam/$mqttClientIdUplinkEvents"
    private val myMqttTopicToPubDenmEvents = "$topicPrefixUpstream/denm/$mqttClientIdUplinkEvents"

    init {
        /*
         * Logging
         */

        Log.d(TAG_MQTT, "MQTT object being initialised with the following:\nITS_MQTT_BROKER_ADDRESS : '$mqttBrokerAddress' \nITS_MQTT_CAM_TOPIC_CLIENT_ID : '$mqttClientIdUplinkEvents'")
        Log.d(TAG_MQTT, "Topics to sub:\nITS_MQTT_UPSTREAM_TOPIC : '$topicPrefixUpstream' \nITS_MQTT_DOWNSTREAM_TOPIC : '$topicPrefixDownstream'")
        Log.d(TAG_MQTT, "Topic to pub:\nMY_CAM_PUB_TOPIC : '$myMqttTopicToPubCamEvents' \nMY_DENM_PUB_TOPIC : '$myMqttTopicToPubDenmEvents'")

        /*
         * Configure MQTT Client
         */

        val qos = 0

        try {
            // Connect options (docs: https://www.eclipse.org/paho/files/javadoc/org/eclipse/paho/client/mqttv3/MqttConnectOptions.html)
            val options = MqttConnectOptions()
            //options.userName = username
            //options.password = password.toCharArray()
            options.connectionTimeout = 60 // Sets the connection timeout value. This value, measured in seconds, defines the maximum time interval the client will wait for the network connection to the MQTT server to be established
            options.keepAliveInterval = 60 // This value, measured in seconds, defines the maximum time interval between messages sent or received. It enables the client to detect if the server is no longer available, without having to wait for the TCP/IP timeout

            /*
             * MQTT Client to receive events
             */
            val mqttClient =
                MqttClient(mqttBrokerAddress, mqttClientIdUplinkEvents, MemoryPersistence())

            // Setup callbacks (docs: https://www.eclipse.org/paho/files/javadoc/org/eclipse/paho/client/mqttv3/MqttCallback.html)
            mqttClient.setCallback(object : MqttCallback {

                // This method is called when the connection to the server is lost
                override fun connectionLost(cause: Throwable) {
                    Log.e(
                        TAG_MQTT,
                        "MQTT connectionLost callback called. Configuration in use:\n" +
                                "ITS_MQTT_BROKER_ADDRESS : '" + mqttBrokerAddress + "' \n" +
                                "ITS_MQTT_CAM_TOPIC_CLIENT_ID : '" + mqttClientIdUplinkEvents + "' \n" +
                                "Topics subscribed:\n" +
                                "ITS_MQTT_UPSTREAM_TOPIC : '" + topicPrefixUpstream + "' \n" +
                                "ITS_MQTT_DOWNSTREAM_TOPIC : '" + topicPrefixDownstream + "' \n" +
                                "Topic to publish:\n" +
                                "MY_CAM_PUB_TOPIC : '" + myMqttTopicToPubCamEvents + "' \n" +
                                "MY_DENM_PUB_TOPIC : '" + myMqttTopicToPubDenmEvents + "' \n" +
                                "Cause message: " + cause.message
                    )
                    cause.printStackTrace()
                    Log.e(TAG_MQTT,"Exiting. Cause: " + cause.message)
                    exitProcess(-1)
                }

                // This method is called when a message arrives from the server
                override fun messageArrived(topic: String?, message: MqttMessage?) {
                    // Evaluation zone: ---------------- ---------------- ----------------
                    if (evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToMobile ||
                        evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToRSU ) {
                        evaluator.networkDelayTransmitterEnd = System.nanoTime()
                    }
                    // ---------------- ---------------- ---------------- ----------------

                    if (topic == myMqttTopicToPubCamEvents || topic == myMqttTopicToPubDenmEvents) {
                        //Log.d(TAG_MQTT, "My own message from MQTT, ignoring.")
                        return
                    }

                    // Evaluation zone: ---------------- ---------------- ----------------
                    if (evaluation == Evaluator.EvaluationEnum.NetworkDelayReceiver) {
                        if (message != null) {
                            CoroutineScope(Dispatchers.IO).launch {
                                val echoMessage = MqttMessage(message.payload)
                                echoMessage.qos = 0
                                mqttClient.publish(myMqttTopicToPubCamEvents, echoMessage)
                            }

                        }
                    }
                    // ---------------- ---------------- ---------------- ----------------

                    // Evaluation zone: ---------------- ---------------- ----------------
                    if (evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToMobile ||
                        evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToRSU) {
                        evaluator.saveTransmissionDelay(evaluationMode = evaluation)
                        return
                    }
                    // ---------------- ---------------- ---------------- ----------------

                    Log.i(TAG_MQTT, "A message has arrived via MQTT.")

                    vmScope.launch{
                        if (message != null) {
                            Log.v(TAG_MQTT, "Payload size: ${message.payload.size}")
                            Log.v(TAG_MQTT, "QoS: ${message.qos}")
                            Log.v(TAG_MQTT, "Payload content: ${message.payload.contentToString()}")
                            receiveService.messageReceived(ITSMessage(message.payload), messagingHistory, evaluator) // Evaluation
                        }
                    }

                }

                // This method is called when delivery for a message has been completed, and all acknowledgments have been received.
                override fun deliveryComplete(token: IMqttDeliveryToken) {
                    Log.v(TAG_MQTT, "MQTT deliveryComplete callback called. Message delivery has been completed, and all acknowledgments have been received. Is token complete? " + token.isComplete)
                }
            })

            /*
             * Connect and subscribe both clients to MQTT server
             */
            mqttClient.connect(options)

            mqttClient.subscribe(topicUpstreamEvents, qos)
            mqttClient.subscribe(topicDownstreamEvents, qos)

            this.mqttClient = mqttClient

        } catch (e: MqttException) {
            Log.e(TAG_MQTT,"MQTT Exception " + e.message)
            e.printStackTrace()
            Log.e(TAG_MQTT,"Exiting.")
            exitProcess(-1)
        }
    }

    override fun sendITSMessage(itsMessage: ITSMessage) {
        // Create message and setup QoS
        val message = MqttMessage(itsMessage.bytes)
        message.qos = 0

        // Publish message
        mqttClient.publish(myMqttTopicToPubCamEvents, message)

        // Evaluation zone: ---------------- ---------------- ----------------
        if (evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToMobile ||
            evaluation == Evaluator.EvaluationEnum.NetworkDelayTransmitterToRSU) {
            evaluator.networkDelayTransmitterStart = System.nanoTime()
        }
        // ---------------- ---------------- ---------------- ----------------

        Log.i(TAG_MQTT,"Message published.")
        Log.d(TAG_MQTT,"message published payload: ${message.payload.contentToString()}")
        Log.d(TAG_MQTT,"message content: ${itsMessage.bytes.contentToString()}")
    }

}