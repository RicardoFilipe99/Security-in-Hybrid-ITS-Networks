package pt.isel.meic.tfm.itsmobileapp.common

import android.content.Context
import android.util.Log
import org.apache.poi.ss.usermodel.Row
import org.apache.poi.ss.usermodel.Sheet
import org.apache.poi.ss.usermodel.Workbook
import org.apache.poi.xssf.usermodel.XSSFWorkbook
import pt.isel.meic.tfm.itsmobileapp.security_protocols.ISecurityProtocol
import java.io.File
import java.io.FileInputStream
import java.io.IOException

private const val TAG_EVALUATION = "ITSMobApp-Eval"

/**
 * Evaluator object to be used in evaluation zones.
 */
class Evaluator(
    private val context: Context,
    ) {

    var networkDelayTransmitterStart : Long = 0
    var networkDelayTransmitterEnd : Long = 0
    lateinit var secProt : ISecurityProtocol

    enum class EvaluationEnum {
        NONE,
        TotalComputationTime,
        SecurityComputationTime,
        NetworkDelayReceiver,
        NetworkDelayTransmitterToMobile,
        NetworkDelayTransmitterToRSU
    }

    /**
     * Save a transmission latency measurement into an Excel file.
     */
    fun saveTransmissionDelay(evaluationMode: EvaluationEnum) {
        val filename = "mobile_app_transmission_delay.xlsx"

        // Calculate total time
        val totalTimeNano = (networkDelayTransmitterEnd - networkDelayTransmitterStart) / 2
        val totalTimeMs = totalTimeNano / 1_000_000.0 // Convert nanoseconds to milliseconds

        writeToExcel(filename = filename, colName = "${evaluationMode}_${secProt.protocolDesignation}", value = totalTimeMs)
    }

    /**
     * Executes a lambda, calculating the processing time and saving it in an Excel file.
     */
    fun executionPerformance(
        executionLambda: () -> Unit,
        identifier: String,
        evaluationMode: EvaluationEnum,
    ) {
        val iterations = 3

        // Execution 'iterations' times
        val startTime = System.nanoTime()

        repeat(iterations) {
            executionLambda()
        }

        val endTime = System.nanoTime()

        // Calculate total time
        val totalTimeNano = endTime - startTime
        val totalTimeMs = totalTimeNano / 1_000_000.0 // Convert nanoseconds to milliseconds

        val averageTimeMs = totalTimeMs / iterations

        val fileToSaveResults: String = if(evaluationMode == EvaluationEnum.TotalComputationTime) {
            "mobile_app_total_computation_time.xlsx"
        } else if(evaluationMode == EvaluationEnum.SecurityComputationTime) {
            "mobile_app_security_computation_time.xlsx"
        } else {
            throw Exception("Unknown evaluation mode: $evaluationMode")
        }

        writeToExcel(filename = fileToSaveResults, colName = identifier, value = averageTimeMs)
    }

    /**
     * Writes a value to an Excel file.
     */
    private fun writeToExcel(
        filename: String,
        colName: String,
        value: Double
    ) {
        //Log.v(TAG_EVALUATION, "Writing in $filename at column $colName the value $value")

        /**
         * Reading / Creating the Workbook
         **/
        val workbook: Workbook

        // Get a reference to the internal storage
        val file = File(context.filesDir, filename)

        workbook = if (file.exists()) {
            val inputStream = FileInputStream(file)
            XSSFWorkbook(inputStream)
        } else {
            XSSFWorkbook()
        }

        if (workbook.getNumberOfSheets() == 0) {
            workbook.createSheet("Evaluation results") // Create a new sheet if none exist
        }

        /**
         * Modify the workbook
         **/
        val sheet: Sheet = workbook.getSheetAt(0) // Get the first sheet, change index if needed

        // Find the specified column's index
        var colIndex = -1
        var headerRow = sheet.getRow(0)

        if (headerRow == null) {

            // Create a header row if none exists
            headerRow = sheet.createRow(0)

        } else {

            // Check if the column exists if an initial row exists
            for (cell in headerRow) {
                if (cell.stringCellValue.equals(colName)) {
                    colIndex = cell.columnIndex
                    break
                }
            }

        }

        if (colIndex == -1) {

            colIndex = headerRow.lastCellNum.toInt() // Next available column index

            if (colIndex == -1)
                colIndex = 0

            val headerCell = headerRow.createCell(colIndex)
            headerCell.setCellValue(colName)
        }

        // Find the next available row for the specified column
        var nextAvailableRow = 1 // Start from the second row (index 1)

        while (nextAvailableRow <= sheet.lastRowNum && sheet.getRow(nextAvailableRow)
                .getCell(colIndex) != null
        ) {
            nextAvailableRow++
        }

        // Create a new row or use the existing one
        var dataRow: Row? = sheet.getRow(nextAvailableRow)
        if (dataRow == null) {
            dataRow = sheet.createRow(nextAvailableRow)
        }

        // Write the value
        dataRow?.createCell(colIndex)?.setCellValue(value)

        /**
         * Save the workbook back to the file
         **/
        try {
            val outputStream = context.openFileOutput(filename, Context.MODE_PRIVATE)
            workbook.write(outputStream)
            outputStream.close()
        } catch (e: IOException) {
            Log.e(TAG_EVALUATION,"IOException. Message: " + e.message)
            e.printStackTrace()
        }
    }
}