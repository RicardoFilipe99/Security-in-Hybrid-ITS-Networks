package pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message

import androidx.compose.runtime.MutableState
import android.util.Log
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.*
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import java.time.Instant

const val TAG_RECEIVE_MESSAGE_SERVICE = "ITSMobApp-RcvService"

/**
 * Implementation of IReceiveMessageService (initial experiments)
 */
class ReceiveMessageServiceImpl(
    private val securityProtocol : ISecurityProtocol,
    private val evaluation : Evaluator.EvaluationEnum,
) : IReceiveMessageService {

    // The following late init vars were not working as expected so a workaround was needed using MqttDispatcher to inject the following vars override late init var evaluator: Evaluator
    //public override late init var messagingHistory: MutableState<List<String>?>

    /**
     * Message handler
     */
    override suspend fun messageReceived(
        itsMessage: ITSMessage,
        // The following vars should be inject by the ViewModel as late init vars, but due to some coroutine / suspend functions problems, a workaround was needed
        messagingHistory: MutableState<List<String>?>,
        evaluator: Evaluator
    ) {
        //Log.d(TAG_RECEIVE_MESSAGE_SERVICE, "Starting processing a received message.")

        fun processMessage() {

            // 1. Verify if message is valid
            val originalMsg = securityProtocol.applySecurityProtocolDecapsulation(itsMessage = itsMessage)

            if (originalMsg == null) {
                Log.d(TAG_RECEIVE_MESSAGE_SERVICE, "Message not verified! returning without updating UI.")
                return
            }

            // 2. Update UI message list
            val protocol = securityProtocol.protocolDesignation

            Log.d(TAG_RECEIVE_MESSAGE_SERVICE, "Updating the UI.")
            updateUiMessageList(
                ScreenMessageOutput(
                    itsMessageContent = originalMsg.bytes,
                    timestamp = Instant.now(),
                    beingSent = false,
                    messageType = MessageType.CAM,
                    securityProtocol = protocol
                ), messagingHistory
            )

            // Evaluation zone: ---------------- ---------------- ----------------
            if (evaluation == Evaluator.EvaluationEnum.SecurityComputationTime) {
                evaluator.executionPerformance(
                    executionLambda = {
                        securityProtocol.applySecurityProtocolDecapsulation(
                            itsMessage = itsMessage
                        )
                    },
                    evaluationMode = evaluation,
                    identifier = "MessageDecapsulation_${securityProtocol.protocolDesignation}[TimesInMs]"
                )
            }
            // ---------------- ---------------- ---------------- ----------------
        }

        processMessage()

        // Evaluation zone: ---------------- ---------------- ----------------
        if (evaluation == Evaluator.EvaluationEnum.TotalComputationTime) {
            evaluator.executionPerformance(
                executionLambda = {
                    processMessage()
                },
                identifier = "FromMQTT_${securityProtocol.protocolDesignation}[TimesInMs]",
                evaluationMode = evaluation,
            )
        }
        // ---------------- ---------------- ---------------- ----------------
    }

    /**
     * Helper function to update UI message history
     */
    private fun updateUiMessageList(message : IMessageOutputModel, messagingHistory: MutableState<List<String>?>) {

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