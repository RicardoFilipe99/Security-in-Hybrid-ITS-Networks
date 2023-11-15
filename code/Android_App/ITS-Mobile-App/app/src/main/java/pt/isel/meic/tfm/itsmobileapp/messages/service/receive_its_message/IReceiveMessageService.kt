package pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message

import androidx.compose.runtime.MutableState
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage

/**
 * Interface Receive Message Service
 */
interface IReceiveMessageService {
    //var messagingHistory: MutableState<List<String>?>
    //var evaluator: Evaluator

    suspend fun messageReceived(
        itsMessage: ITSMessage,
        messagingHistory: MutableState<List<String>?>, // This argument only exists due to a bug
        evaluator: Evaluator                           // This argument only exists due to a bug
    )
}