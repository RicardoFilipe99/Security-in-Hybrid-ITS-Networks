package pt.isel.meic.tfm.itsmobileapp.messages.ui

import android.util.Log
import androidx.compose.foundation.layout.*
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import pt.isel.meic.tfm.itsmobileapp.messages.ui.views.MessageTextView
import pt.isel.meic.tfm.itsmobileapp.messages.ui.views.SendCAMButtonView
import pt.isel.meic.tfm.itsmobileapp.messages.ui.views.SendDENMButtonView
import pt.isel.meic.tfm.itsmobileapp.messages.ui.views.StopCAMButtonView
import pt.isel.meic.tfm.itsmobileapp.ui.theme.ITSMobileAppTheme

const val TAG_MESSAGES_UI = "ITSMobApp-MsgUI"

@Composable
fun MessageScreen(
    messagingHistory: List<String>?,
    sendingCAM : Boolean,
    sendingDENM : Boolean,
    onSendCamButtonClick : () -> Unit,
    onStopCamButtonClick : () -> Unit,
    onSendDenmButtonClick : () -> Unit,
) {

    //Log.v(TAG_MESSAGES_UI, "MessageScreen composed")

    ITSMobileAppTheme {
        // A surface container using the 'background' color from the theme
        Surface(
            modifier = Modifier
                .fillMaxSize(),
            color = MaterialTheme.colors.background
        ) {

            Box( // The box layout only has 1 child
                contentAlignment = Alignment.Center,
                modifier = Modifier
                    .fillMaxSize()
            ) {
                MessageTextView(messagingHistory)
            }

            Row (
                verticalAlignment = Alignment.Bottom
            ) { // It is necessary do specify how the components are shown in the screen

                /** Send CAM Button **/
                SendCAMButtonView(
                    enabled = !sendingCAM,
                    onClick = onSendCamButtonClick,
                    modifier = Modifier
                        .padding(all = 8.dp)
                )

                /** Stop CAM Button **/
                StopCAMButtonView(
                    enabled = sendingCAM,
                    onClick = onStopCamButtonClick,
                    modifier = Modifier
                        .padding(all = 8.dp)
                )

                /** Send DENM Button **/
                SendDENMButtonView(
                    enabled = !sendingDENM,
                    onClick = onSendDenmButtonClick,
                    modifier = Modifier
                        .padding(all = 8.dp)
                        .padding(start = 48.dp)
                )

            }
        }
    }
}

/*** Button disabled ***/
@Preview(showBackground = true)
@Composable
fun PreviewMessageScreen() {
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

    ITSMobileAppTheme {
        MessageScreen(
            messagingHistory = messagingHistoryList,
            sendingCAM = true,
            sendingDENM = true,
            onSendCamButtonClick = {},
            onStopCamButtonClick = {},
            onSendDenmButtonClick = {},
        )
    }

}
