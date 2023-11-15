package pt.isel.meic.tfm.itsmobileapp.messages.ui

import android.util.Log
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.State
import androidx.compose.runtime.mutableStateOf
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.launch
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.service.receive_its_message.IReceiveMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message.ISendMessageService
import pt.isel.meic.tfm.itsmobileapp.messages.service.dispatcher.IMessageDispatcher
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol

class MessagingViewModel(
        private val sendService: ISendMessageService,
        private val receiveService: IReceiveMessageService,
        private val messageDispatcher: IMessageDispatcher,
        private val evaluator: Evaluator,
        private val securityProt: ISecurityProtocol,
    ) : ViewModel() {

    private val syncObj : Any = Any()

    private val _messagingHistory : MutableState<List<String>?> = mutableStateOf(null) // mutableStateOf(listOf("")) //

    val messagingHistory : State<List<String>?>
        get() = _messagingHistory

    private val _sendingCAM : MutableState<Boolean> = mutableStateOf(false)
    val sendingCAM : State<Boolean>
        get() = _sendingCAM

    private val _sendingDENM : MutableState<Boolean> = mutableStateOf(false)
    val sendingDENM : State<Boolean>
        get() = _sendingDENM

    init {
        evaluator.secProt = securityProt
        //Log.d(TAG_MESSAGES_UI, "View model initializing.")

        messageDispatcher.vmScope = viewModelScope
        messageDispatcher.messagingHistory = _messagingHistory
        messageDispatcher.evaluator = evaluator

        //receiveService.messagingHistory = _messagingHistory // Gives an error 'lateinit property evaluator has not been initialized'
        //receiveService.evaluator = evaluator // Gives an error 'lateinit property evaluator has not been initialized'

        sendService.messagingHistory = _messagingHistory
        sendService.evaluator = evaluator
    }

    fun onSendCamButtonClick() {

        viewModelScope.launch { // Coroutine whose lifetime is subordinate to the ViewModel itself
            //Log.d(TAG_MESSAGES_UI, "onSendCamButtonClick")

            synchronized(syncObj) {
                _sendingCAM.value = true
                sendService.keepSendingCAM = true
            }

            sendService.sendCAM()
        }

    }

    fun onStopCamButtonClick() {

        viewModelScope.launch {  // Coroutine whose lifetime is subordinate to the ViewModel itself
            //Log.d(TAG_MESSAGES_UI, "onStopCamButtonClick")

            synchronized(syncObj) {
                _sendingCAM.value = false
                sendService.keepSendingCAM = false
            }

        }

    }

    fun onSendDenmButtonClick() {

        viewModelScope.launch {  // Coroutine whose lifetime is subordinate to the ViewModel itself
            //Log.d(TAG_MESSAGES_UI, "onSendDenmButtonClick")
            _sendingDENM.value = true
            sendService.sendDENM()
            _sendingDENM.value = false
        }

    }

}