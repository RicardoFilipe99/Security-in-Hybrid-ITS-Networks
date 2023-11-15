package pt.isel.meic.tfm.itsmobileapp.messages.ui.views

import android.util.Log
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import pt.isel.meic.tfm.itsmobileapp.R
import pt.isel.meic.tfm.itsmobileapp.messages.ui.TAG_MESSAGES_UI
import pt.isel.meic.tfm.itsmobileapp.ui.theme.ITSMobileAppTheme

@Composable
fun SendDENMButtonView(
    enabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier // By default is a empty list. Every composable should have a modifier (common stuff to all). So if I have created my costume one that uses a button, I should pass it through
) {
    //Log.v(TAG_MESSAGES_UI, "SendDENMButtonView composed")

    Button(
        enabled = enabled,
        onClick = onClick,
        modifier = modifier
    ) {
        val text = stringResource(id = R.string.activity_main_send_denm_button_text)
        Text(text = text)
    }
}

/**
 * Previews
 */

// Preview DENM button enabled
@Preview(showBackground = true)
@Composable
fun PreviewSendDENMButtonViewEnabled() {
    ITSMobileAppTheme {
        SendCAMButtonView(
            enabled = true,
            onClick = {},
            Modifier
                .padding(all = 16.dp)
        )
    }
}

// Preview DENM button disabled
@Preview(showBackground = true)
@Composable
fun PreviewSendDENMButtonViewDisabled() {
    ITSMobileAppTheme {
        SendCAMButtonView(
            enabled = false,
            onClick = {},
            Modifier
                .padding(all = 16.dp)
        )
    }
}