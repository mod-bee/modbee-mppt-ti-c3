/*!
 * @file ModbeeMpptDebug.h
 * 
 * @brief Debug and diagnostic functions for ModbeeMPPT
 * 
 * This module provides clean, organized debug output for the MPPT controller,
 * including measurements, configuration, status, and fault information.
 */

#ifndef MODBEE_MPPT_DEBUG_H
#define MODBEE_MPPT_DEBUG_H

#include "ModbeeMpptGlobal.h"

// Forward declarations
class ModbeeMPPT;
class ModbeeMpptAPI;

/*!
 * @brief Debug and diagnostic utility class for ModbeeMPPT
 * 
 * Provides organized, readable debug output for all MPPT controller information
 * including power measurements, configuration settings, status registers, and faults.
 */
class ModbeeMpptDebug {
public:
  /*!
   * @brief Constructor
   * @param mppt Reference to the ModbeeMPPT instance
   */
  ModbeeMpptDebug(ModbeeMPPT& mppt);
  
  /*!
   * @brief Destructor
   */
  ~ModbeeMpptDebug();

  // ========================================================================
  // COMPREHENSIVE DEBUG OUTPUT
  // ========================================================================
  
  /*!
   * @brief Print complete system status in organized sections
   * 
   * Prints all measurements, configuration, status, and faults in a
   * clean, easy-to-read format with clear section headers.
   */
  void printCompleteStatus();

  // ========================================================================
  // INDIVIDUAL SECTION PRINTING
  // ========================================================================
  
  /*!
   * @brief Print power measurements section
   * 
   * Shows input, battery, system power with voltages, currents, and calculated power.
   * Includes efficiency and individual ADC readings.
   */
  void printPowerMeasurements();
  
  /*!
   * @brief Print comprehensive battery status with all voltage and SOC readings
   * 
   * Shows charging voltage, true battery voltage, actual SOC, usable SOC,
   * current, temperature, and battery state information.
   */
  void printComprehensiveBatteryStatus();
  
  /*!
   * @brief Print current configuration settings
   * 
   * Shows all configurable parameters like charge voltage/current limits,
   * input limits, timers, ADC settings, MPPT configuration, etc.
   */
  void printConfiguration();
  
  /*!
   * @brief Print status information
   * 
   * Shows charging state, power path status, individual status register
   * contents, and operational status indicators.
   */
  void printStatus();
  
  /*!
   * @brief Print fault information
   * 
   * Shows any active faults with detailed descriptions and diagnostic
   * information to help troubleshoot issues.
   */
  void printFaults();
  
  /*!
   * @brief Print power path diagnostics
   * 
   * Shows detailed power path information including FET states,
   * backup mode, ship mode, and power routing analysis.
   */
  void printPowerPathDiagnostics();

  // ========================================================================
  // RAW REGISTER DEBUG
  // ========================================================================
  
  /*!
   * @brief Print raw register values
   * 
   * Shows all status and fault registers in hexadecimal format
   * for low-level debugging.
   */
  void printRawRegisters();
  
  /*!
   * @brief Print detailed register decoding
   * 
   * Shows bit-by-bit breakdown of status and fault registers
   * with descriptions of each bit's meaning.
   */
  void printRegisterDecoding();

  // ========================================================================
  // UTILITY FUNCTIONS
  // ========================================================================
  
  /*!
   * @brief Print a formatted section header
   * @param title Section title
   * @param width Header width (default 60)
   */
  void printSectionHeader(const String& title, int width = 60);
  
  /*!
   * @brief Print a formatted main header with emphasis
   * @param title Main header title
   * @param width Header width (default 80)
   */
  void printMainHeader(const String& title, int width = 80);
  
  /*!
   * @brief Print a section divider line
   */
  void printSectionDivider();
  
  /*!
   * @brief Print a formatted subsection header
   * @param title Subsection title
   */
  void printSubsectionHeader(const String& title);
  
  /*!
   * @brief Format a boolean status for display
   * @param label Status label
   * @param status Boolean value
   * @return Formatted string
   */
  String formatStatus(const String& label, bool status);
  
  /*!
   * @brief Format a boolean status for display (clean format)
   * @param status Boolean value
   * @return Formatted string (just "ENABLED"/"DISABLED")
   */
  String formatStatusClean(bool status);
  
  /*!
   * @brief Format a field name and value with consistent alignment
   * @param fieldName Name of the field
   * @param value Value to display
   * @return Formatted string with proper alignment
   */
  String formatField(const String& fieldName, const String& value);

private:
  ModbeeMPPT& _mppt;        // Reference to MPPT instance
  
  // Helper functions for specific diagnostic areas
  void printBatteryInfo();
  void printInputSourceInfo();
  void printSystemInfo();
  void printTemperatureInfo();
  void printTimerInfo();
  void printMPPTInfo();
  void printADCInfo();
  void printProtectionInfo();
};

#endif // MODBEE_MPPT_DEBUG_H
