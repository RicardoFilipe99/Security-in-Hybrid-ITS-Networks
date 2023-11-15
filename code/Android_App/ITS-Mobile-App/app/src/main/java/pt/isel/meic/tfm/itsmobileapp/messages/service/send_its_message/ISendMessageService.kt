package pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message

import androidx.compose.runtime.MutableState
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator

/**
 * Interface Send Message Service
 */
interface ISendMessageService {
    var keepSendingCAM : Boolean
    var messagingHistory: MutableState<List<String>?>
    var evaluator: Evaluator

    suspend fun sendCAM()
    suspend fun sendDENM()
}