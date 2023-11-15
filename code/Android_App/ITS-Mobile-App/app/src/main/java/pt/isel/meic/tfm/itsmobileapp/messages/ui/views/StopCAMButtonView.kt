package pt.isel.meic.tfm.itsmobileapp.messages.ui.views

import android.util.Log
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Button
import androidx.compose.material.ButtonDefaults
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import pt.isel.meic.tfm.itsmobileapp.R
import pt.isel.meic.tfm.itsmobileapp.messages.ui.TAG_MESSAGES_UI
import pt.isel.meic.tfm.itsmobileapp.ui.theme.ITSMobileAppTheme

@Composable
fun StopCAMButtonView(
    enabled: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier // By default is a empty list. Every composable should have a modifier (common stuff to all). So if I have created my costume one that uses a button, I should pass it through
) {
    //Log.v(TAG_MESSAGES_UI, "StopCAMButtonView composed")

    Button(
        enabled = enabled,
        onClick = onClick,
        modifier = modifier,
        colors = ButtonDefaults.buttonColors(backgroundColor = Color.Red)
    ) {
        val text = stringResource(id = R.string.activity_main_stop_cam_button_text)
        Text(text = text)
    }
}

/**
 * Previews
 */

// Preview Stop CAM button disabled
@Preview(showBackground = true)
@Composable
fun PreviewStopCAMButtonViewDisabled() {
    ITSMobileAppTheme {
        StopCAMButtonView(
            enabled = false,
            onClick = {},
            Modifier
                .padding(all = 16.dp)
        )
    }
}

// Preview Stop CAM button enabled
@Preview(showBackground = true)
@Composable
fun PreviewStopCAMButtonViewEnabled() {
    ITSMobileAppTheme {
        StopCAMButtonView(
            enabled = true,
            onClick = {},
            Modifier
                .padding(all = 16.dp)
        )
    }
}

