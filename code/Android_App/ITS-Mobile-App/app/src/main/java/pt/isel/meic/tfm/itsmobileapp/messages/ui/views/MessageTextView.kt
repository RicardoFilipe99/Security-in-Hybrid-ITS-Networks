package pt.isel.meic.tfm.itsmobileapp.messages.ui.views

import android.util.Log
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import pt.isel.meic.tfm.itsmobileapp.R
import pt.isel.meic.tfm.itsmobileapp.messages.ui.TAG_MESSAGES_UI
import pt.isel.meic.tfm.itsmobileapp.ui.theme.ITSMobileAppTheme

@Composable
fun MessageTextView(
    messagingHistory: List<String>?
) {
    //Log.v(TAG_MESSAGES_UI, "MessageTextView composed")

    Column {

        // Title
        Text(
            style = MaterialTheme.typography.h5,
            text = stringResource(id = R.string.view_message_text_title),
            textAlign = TextAlign.Center,
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 24.dp)
        )

        // Messages
        if (messagingHistory != null) {
            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(500.dp)
                    .padding(all = 16.dp),
                verticalArrangement = Arrangement.Top,
                contentPadding = PaddingValues(bottom = 24.dp),
                userScrollEnabled = true
            ) {
                items(messagingHistory) { item ->
                    Text(
                        text = item,
                        modifier = Modifier.padding(vertical = 8.dp),
                    )
                }
            }

        } else {
            Text(
                text = stringResource(id = R.string.view_message_text_default_message),
                style = MaterialTheme.typography.subtitle1,
                textAlign = TextAlign.Center,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(bottom = 24.dp)
            )
        }

    }

}

/**
 * Previews
 */

// Preview with messages
@Preview
@Composable
fun PreviewMessageTextViewWithMessages() {
    ITSMobileAppTheme {
        MessageTextView(messagesListExample)
    }
}

// Preview without messages
@Preview
@Composable
fun PreviewMessageTextViewWithoutMessages() {
    ITSMobileAppTheme {
        MessageTextView(null)
    }
}

val messagesListExample = listOf(
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