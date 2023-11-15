package pt.isel.meic.tfm.itsmobileapp.messages.service.send_its_message

import androidx.compose.runtime.MutableState
import kotlinx.coroutines.delay
import pt.isel.meic.tfm.itsmobileapp.common.Evaluator
import pt.isel.meic.tfm.itsmobileapp.messages.models.IMessageOutputModel
import java.time.Instant
import java.time.format.DateTimeFormatter

/**
 * Fake implementation of ISendMessageService (initial experiments)
 */
class SendMessageServiceFakeImpl() : ISendMessageService {

    public override var keepSendingCAM : Boolean = false
    public override lateinit var messagingHistory: MutableState<List<String>?>
    public override var evaluator: Evaluator
        get() = TODO("Not yet implemented")
        set(value) {}
    private val syncObj : Any = Any()

    override suspend fun sendCAM() {
        delay(2000) // Give a loading time effect
        updateUiMessageList(FakeCAMMessage())
    }

    override suspend fun sendDENM() {
        delay(2000) // Give a loading time effect
        updateUiMessageList(FakeDENMMessage())
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

    class FakeCAMMessage : IMessageOutputModel {
        override val content: String
            get() = "Fake CAM being sent at: " + DateTimeFormatter.ISO_INSTANT.format(Instant.now())
    }

    class FakeDENMMessage : IMessageOutputModel {
        override val content: String
            get() = "Fake CAM being sent at: " + DateTimeFormatter.ISO_INSTANT.format(Instant.now())
    }

    /**
     * Fake message list
     */
    val messagingHistoryList = listOf(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
        "-------------------------------------------------------------------",
        "Donec sit amet velit et sapien pulvinar bibendum in vel sem.",
        "-------------------------------------------------------------------",
        "Curabitur rutrum ex eget nulla aliquet facilisis.",
        "-------------------------------------------------------------------",
        "Praesent pulvinar dignissim diam, id euismod urna varius sit amet.",
        "-------------------------------------------------------------------",
        "Etiam maximus finibus mi, at lobortis ipsum dignissim sit amet.",
        "-------------------------------------------------------------------",
        "Nullam lacinia nisl sed vestibulum pretium.",
        "-------------------------------------------------------------------",
        "Nam gravida, magna et auctor dapibus, augue elit sollicitudin metus, nec commodo mauris urna vitae nisi.",
        "-------------------------------------------------------------------",
        "Cras ac mauris vitae mi rhoncus tincidunt.",
        "-------------------------------------------------------------------",
        "Aliquam in purus aliquam, imperdiet metus nec, auctor urna.",
        "-------------------------------------------------------------------",
        "Fusce eu ex blandit, consectetur nibh ut, efficitur nunc.",
        "-------------------------------------------------------------------",
        "Suspendisse feugiat, odio id venenatis luctus, libero metus placerat nisl, in efficitur nisl dolor ac lectus.",
        "Vivamus vel risus ultrices, bibendum velit ac, interdum sapien.",
        "-------------------------------------------------------------------",
        "Sed a nulla eget enim porttitor hendrerit ac nec lacus.",
        "-------------------------------------------------------------------",
        "Morbi eleifend lacus vel nisi tincidunt, sit amet semper odio luctus.",
    )

}