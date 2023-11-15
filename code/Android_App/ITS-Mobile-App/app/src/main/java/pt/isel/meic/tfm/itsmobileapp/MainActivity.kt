package pt.isel.meic.tfm.itsmobileapp

import android.content.res.AssetManager
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.common.TAG
import pt.isel.meic.tfm.itsmobileapp.messages.ui.MessageScreen
import pt.isel.meic.tfm.itsmobileapp.messages.ui.MessagingViewModel


class MainActivity : ComponentActivity() { // Is the entry point activity (defined in the manifest android:name=".MainActivity")

    companion object {
        lateinit var assetManager: AssetManager
    }

    /**
     * Objects to be used by the activity's ViewModel
     */

    private val sendMessagesService by lazy {
        (application as DependenciesContainer).sendMessageService
    }

    private val receiveMessagesService by lazy {
        (application as DependenciesContainer).receiveMessageService
    }

    private val messageDispatcher by lazy {
        (application as DependenciesContainer).messageDispatcher
    }

    private val securityProt by lazy {
        (application as DependenciesContainer).securityProt
    }


    /*private val fileManager by lazy {
        (application as DependenciesContainer).fileManager
    }


    val fileManager = FileManager(applicationContext)*/

    /**
     * Create ViewModel
     */

    @Suppress("UNCHECKED_CAST")
    private val vm by viewModels<MessagingViewModel> {
        object : ViewModelProvider.Factory { // Factory object whose create method is called to instantiate the viewModel
            override fun <T : ViewModel> create(modelClass: Class<T>): T {
                return MessagingViewModel(
                    sendService = sendMessagesService,
                    receiveService = receiveMessagesService,
                    messageDispatcher = messageDispatcher,
                    evaluator = Evaluator(applicationContext),
                    securityProt = securityProt
                ) as T
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        Log.v(TAG, application.javaClass.name)

        assetManager = resources.assets

        setContent {
            Log.v(TAG, "Composing activity content")

            // Access to the .value of the states, has to be within the setContent which is a composable
            val messagingHistoryList = vm.messagingHistory
            val sendingCAM = vm.sendingCAM
            val sendingDENM = vm.sendingDENM

            MessageScreen (
                messagingHistory = messagingHistoryList.value,
                sendingCAM = sendingCAM.value,
                sendingDENM = sendingDENM.value,
                onSendCamButtonClick = { vm.onSendCamButtonClick() },
                onStopCamButtonClick = { vm.onStopCamButtonClick() },
                onSendDenmButtonClick  = { vm.onSendDenmButtonClick() },
            )

        }

    }

}

