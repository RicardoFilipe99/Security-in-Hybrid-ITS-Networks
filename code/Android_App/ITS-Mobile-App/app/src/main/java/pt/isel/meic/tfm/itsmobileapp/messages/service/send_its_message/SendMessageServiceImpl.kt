package pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message

import androidx.compose.runtime.MutableState
import kotlinx.coroutines.delay
import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.*
import pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher.IMessageDispatcher
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.message_generator.IMessageGenerator
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import java.time.Instant

const val TAG_SEND_MESSAGE_SERVICE = "ITSMobApp-SendService"

/**
 * Implementation of ISendMessageService
 */
data class SendMessageServiceImpl(
    private val messageGenerator : IMessageGenerator,
    val securityProtocol : ISecurityProtocol,
    private val dispatcher : IMessageDispatcher?,
    private val camFrequency : Double, // Hz
    private val evaluation : Evaluator.EvaluationEnum,
) : ISendMessageService {

    private val camPeriodInMillis : Long = ( (1 / camFrequency) * 1000 ).toLong()
    override var keepSendingCAM : Boolean = false
    override lateinit var messagingHistory: MutableState<List<String>?>
    override lateinit var evaluator: Evaluator // Evaluation

    override suspend fun sendCAM() {

        Log.i(TAG_SEND_MESSAGE_SERVICE, "Sending CAMs periodically every $camPeriodInMillis millis.")

        fun sendMessage() {
            // 1. Generate message
            var cam = messageGenerator.generateCAM()

            // 2. Protect message (apply protocol)
            cam = securityProtocol.applySecurityProtocolEncapsulation(itsMessage = cam)

            // 3. Dispatch message
            dispatcher?.sendITSMessage(cam)

            // 4. Update UI message list
            val protocol = securityProtocol.protocolDesignation

            Log.d(TAG_SEND_MESSAGE_SERVICE, "Updating the UI.")
            updateUiMessageList(
                ScreenMessageOutput(
                    itsMessageContent = cam.bytes,
                    timestamp = Instant.now(),
                    beingSent = true,
                    messageType = MessageType.CAM,
                    securityProtocol = protocol
                )
            )

            // Evaluation zone: ---------------- ---------------- ----------------
            if (evaluation == Evaluator.EvaluationEnum.SecurityComputationTime) {
                evaluator.executionPerformance(
                    executionLambda = {
                        securityProtocol.applySecurityProtocolEncapsulation(
                            itsMessage = cam
                        )
                    },
                    identifier = "MessageEncapsulation_${securityProtocol.protocolDesignation}[TimesInMs]",
                    evaluationMode = evaluation,
                )
            }
            // ---------------- ---------------- ---------------- ----------------

        }

        while (keepSendingCAM) {

            sendMessage()

            // Evaluation zone: ---------------- ---------------- ----------------
            if (evaluation == Evaluator.EvaluationEnum.TotalComputationTime) {
                evaluator.executionPerformance(
                    executionLambda = {
                        sendMessage()
                    },
                    identifier = "GeneratedToMQTT_${securityProtocol.protocolDesignation}[TimesInMs]",
                    evaluationMode = evaluation,
                )
            }
            // ---------------- ---------------- ---------------- ----------------

            delay(camPeriodInMillis)
        }
    }

    override suspend fun sendDENM() {
        TODO("Not yet implemented")
    }

    /**
     * Helper function to update UI message history
     */
    private fun updateUiMessageList(message : IMessageOutputModel) {

        synchronized(messagingHistory) { // Access to messaging history can be concurrent

            if (messagingHistory.value == null) {

                messagingHistory.value = listOf(
                    message.content,
                    "-------------------------------------------------------------------",
                )

            } else {
                val temp : List<String> = messagingHistory.value!!

                messagingHistory.value = listOf(
                    message.content,
                    "-------------------------------------------------------------------",
                ) + temp

            }
        }

    }

}




