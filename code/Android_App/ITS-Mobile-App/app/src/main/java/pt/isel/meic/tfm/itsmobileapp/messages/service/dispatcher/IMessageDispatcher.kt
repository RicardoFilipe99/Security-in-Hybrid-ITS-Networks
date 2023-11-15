package pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher

import androidx.compose.runtime.MutableState
import kotlinx.coroutines.CoroutineScope
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.ITSMessage
import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.IReceiveMessageService

/**
 * Interface definition for an ITS Message Dispatcher
 */
interface IMessageDispatcher {
    abstract var evaluator: Evaluator
    var vmScope: CoroutineScope
    val receiveService : IReceiveMessageService
    var messagingHistory: MutableState<List<String>?>

    fun sendITSMessage(itsMessage: ITSMessage)
}