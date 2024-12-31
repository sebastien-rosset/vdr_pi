/******************************************************************************
 * $Id: vdr_pi.h, v0.2 2011/05/23 SethDart Exp $
 *
 * Project:  OpenCPN
 * Purpose:  VDR Plugin
 * Author:   Jean-Eudes Onfray
 *
 ***************************************************************************
 *   Copyright (C) 2011 by Jean-Eudes Onfray   *
 *   $EMAIL$   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef _VDRPI_H_
#define _VDRPI_H_

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // precompiled headers

#include <wx/fileconf.h>
#include <wx/filepicker.h>
#include <wx/file.h>
#include <wx/aui/aui.h>
#include <wx/radiobut.h>

#include "ocpn_plugin.h"

#include "config.h"

#define VDR_TOOL_POSITION -1  // Request default positioning of toolbar tool

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

class VDRControl;

/**
 * Data storage formats supported by the VDR plugin.
 *
 * Controls how data is structured and stored in VDR files. Each format offers
 * different capabilities for data organization and playback control.
 */
enum class VDRDataFormat {
  RawNMEA,  //!< Raw NMEA sentences stored unmodified
  CSV,  //!< Structured CSV format with timestamps and message type metadata.
        // Future formats can be added here
};

/**
 * Protocol recording configuration settings.
 *
 * Controls which maritime data protocols are captured during recording.
 * Multiple protocols can be enabled simultaneously.
 */
struct VDRProtocolSettings {
  bool nmea0183;  //!< Enable NMEA 0183 sentence recording
  bool nmea2000;  //!< Enable NMEA 2000 PGN message recording
  bool signalK;   //!< Enable Signal K data recording
};

/**
 * Column definition for CSV format files.
 *
 * Describes the structure and requirements of fields in CSV format
 * VDR files to support proper parsing during playback.
 */
struct CSVField {
  wxString name;  //!< Field name in CSV header
  int index;      //!< Zero-based column position in CSV file
  bool required;  //!< Field must be present for valid playback.
};

wxDECLARE_EVENT(EVT_N2K, ObservedEvt);
wxDECLARE_EVENT(EVT_SIGNALK, ObservedEvt);

/**
 * Voyage Data Recorder plugin for OpenCPN
 *
 * Records maritime data including NMEA 0183, NMEA 2000 and SignalK messages to
 * files for later playback. Supports automatic recording based on vessel speed
 * and automatic file rotation for data management.
 */
class vdr_pi : public opencpn_plugin_118 {
public:
  /** Creates a new VDR plugin instance. */
  vdr_pi(void* ppimgr);

  /** Initializes the plugin and sets up toolbar items. */
  int Init(void);
  /** Cleans up resources and saves configuration. */
  bool DeInit(void);

  int GetAPIVersionMajor() override;
  int GetAPIVersionMinor() override;
  int GetPlugInVersionMajor() override;
  int GetPlugInVersionMinor() override;
  int GetPlugInVersionPatch() override;
  int GetPlugInVersionPost() override;
  const char* GetPlugInVersionPre() override;
  const char* GetPlugInVersionBuild() override;
  wxBitmap* GetPlugInBitmap();
  wxString GetCommonName() override;
  wxString GetShortDescription();
  wxString GetLongDescription();

  /**
   * Process timer notification for playback events.
   *
   * Handles timed playback of recorded data, managing message timing
   * and maintaining playback state.
   */
  void Notify();
  /**
   * Set the interval for timer notifications.
   * @param interval Timer interval in milliseconds
   */
  void SetInterval(int interval);

  /**
   * Process an incoming NMEA 0183 sentence for recording.
   *
   * Records the sentence if recording is active and NMEA 0183 is enabled.
   * For RMC sentences, also processes vessel speed for auto-recording.
   * @param sentence NMEA sentence to process
   */
  void SetNMEASentence(wxString& sentence);
  /**
   * Process an incoming AIS message for recording.
   *
   * Records AIS messages similarly to NMEA sentences if recording is active.
   * @param sentence AIS message to process
   */
  void SetAISSentence(wxString& sentence);
  /**
   * Get number of toolbar items added by plugin.
   * @return Number of toolbar items
   */
  int GetToolbarToolCount(void);
  /** Handle toolbar button clicks. */
  void OnToolbarToolCallback(int id);
  /** Update the plugin's color scheme .*/
  void SetColorScheme(PI_ColorScheme cs);

  /** Load a VDR file containing NMEA data, either in raw NMEA format or CSV. */
  bool LoadFile(const wxString& filename);
  /** Start recording VDR data. */
  void StartRecording();
  /** Stop recording VDR data and close the VDR file. */
  void StopRecording(const wxString& reason = wxEmptyString);
  /** Pause recording VDR data, retain the existing VDR file. */
  void PauseRecording(const wxString& reason = wxEmptyString);
  /** Resume recording using the same VDR file. */
  void ResumeRecording();
  /** Start playback of VDR data. */
  void StartPlayback();
  /** Pause playback of VDR data. */
  void PausePlayback();
  /** Stop playback of VDR data. */
  void StopPlayback();
  /** Return whether recording is currently active. */
  bool IsRecording() { return m_recording; }
  /** Return whether playback is currently active. */
  bool IsPlaying() { return m_playing; }
  /** Return whether the end of the playback file has been reached. */
  bool IsAtFileEnd() const { return m_atFileEnd; }
  void ResetEndOfFile() { m_atFileEnd = false; }
  /** Schedule the next playback message based on the message timestamp. */
  void ScheduleNextPlayback();

  /** Invoked during playback. */
  void OnTimer(wxTimerEvent& event);

  /** Show the plugin preferences dialog. */
  void ShowPreferencesDialog(wxWindow* parent);
  /** Get current data format setting for VDR output */
  VDRDataFormat GetDataFormat() const { return m_data_format; }
  /**
   * Set data format for VDR recording.
   *
   * If recording is active, stops current recording and starts new one with
   * updated format.
   * @param dataFormat New format to use for recording.
   */
  void SetDataFormat(VDRDataFormat dataFormat);
  /** Get configured recording directory. */
  wxString GetRecordingDir() const { return m_recording_dir; }
  /**
   * Set directory for storing VDR recordings.
   * @param dir Path to recording directory
   */
  void SetRecordingDir(const wxString& dir) { m_recording_dir = dir; }
  /**
   * Generate filename for new recording based on current UTC time.
   *
   * Creates filename in format: vdr_YYYYMMDDTHHMMSSz with appropriate
   * extension.
   */
  wxString GenerateFilename() const;

  /** Check if automatic log rotation is enabled. */
  bool IsLogRotateEnabled() const { return m_log_rotate; }
  /**
   * Enable or disable automatic log rotation.
   * @param enable True to enable rotation
   */
  void SetLogRotate(bool enable) { m_log_rotate = enable; }
  /** Get configured interval between log rotations in hours. */
  int GetLogRotateInterval() const { return m_log_rotate_interval; }
  /**
   * Set interval for automatic log rotation.
   * @param hours Hours between rotations
   */
  void SetLogRotateInterval(int hours) { m_log_rotate_interval = hours; }
  /** Check if current recording file needs rotation based on elapsed time. */
  void CheckLogRotation();
  /**
   * Update toolbar button state.
   * @param id Toolbar item identifier
   * @param status New enabled/pressed state
   */
  void SetToolbarToolStatus(int id, bool status);
  /** Get identifier for play button toolbar item. */
  int GetPlayToolbarItemId() const { return m_tb_item_id_play; }

  /**
   * Scan loaded file for timestamp information.
   *
   * Analyzes file to determine if it contains valid timestamps and stores
   * first/last timestamps if found. Required for proper playback timing.
   * @return True if valid timestamps found
   */
  bool ScanFileTimestamps();
  /**
   * Seek playback position to specified fraction of file.
   *
   * For files with timestamps, seeks to matching timestamp.
   * For files without timestamps, seeks to line number.
   * @param fraction Position as fraction between 0-1
   * @return True if seek successful
   */
  bool SeekToFraction(double fraction);
  /**
   * Get current playback position as fraction of total.
   *
   * For files with timestamps, based on timestamp position.
   * For files without timestamps, based on line position.
   * @return Position as fraction between 0-1
   */
  double GetProgressFraction() const;
  /** Get timestamp of first message in file. */
  wxDateTime GetFirstTimestamp() const { return m_firstTimestamp; }
  /** Get timestamp of last message in file. */
  wxDateTime GetLastTimestamp() const { return m_lastTimestamp; }
  /** Get timestamp at current playback position. */
  wxDateTime GetCurrentTimestamp() const { return m_currentTimestamp; }
  /**
   * Set timestamp for current playback position.
   * @param timestamp New current timestamp
   */
  void SetCurrentTimestamp(const wxDateTime& timestamp) {
    m_currentTimestamp = timestamp;
  }
  /**
   * Get path of currently loaded input file.
   *
   * Returns empty string if no file loaded or file doesn't exist.
   */
  wxString GetInputFile() const;
  /** Clear current input file reference and close file if open. */
  void ClearInputFile();
  /**
   * Adjust playback timing based on speed multiplier setting.
   *
   * Updates base time to maintain proper message timing when
   * playback speed is changed.
   */
  void AdjustPlaybackBaseTime();
  /** Check if automatic recording start is enabled. */
  bool IsAutoStartRecording() const { return m_auto_start_recording; }
  /**
   * Enable or disable automatic recording start.
   * @param enable True to enable auto-start
   */
  void SetAutoStartRecording(bool enable) { m_auto_start_recording = enable; }
  /** Check if speed threshold for recording is enabled. */
  bool IsUseSpeedThreshold() const { return m_use_speed_threshold; }
  /**
   * Enable or disable speed threshold for recording.
   * @param enable True to enable threshold
   */
  void SetUseSpeedThreshold(bool enable) { m_use_speed_threshold = enable; }
  /** Get configured speed threshold in knots. */
  double GetSpeedThreshold() const { return m_speed_threshold; }
  /**
   * Set speed threshold for automatic recording.
   * @param threshold Speed threshold in knots
   */
  void SetSpeedThreshold(double threshold) { m_speed_threshold = threshold; }
  /** Get configured delay before stopping recording. */
  int GetStopDelay() const { return m_stop_delay; }
  /**
   * Set delay before stopping recording when speed drops.
   * @param minutes Minutes to wait before stopping
   */
  void SetStopDelay(int minutes) { m_stop_delay = minutes; }
  /**
   * Check if auto-recording should be started or stopped based on boat speed.
   *
   * Starts recording when speed exceeds threshold and stops recording
   * after configured delay when speed drops below threshold.
   * @param speed Current boat speed in knots
   */
  void CheckAutoRecording(double speed);
  /**
   * Check if current file contains valid message timestamps.
   *
   * File must have monotonically increasing timestamps for
   * timestamp-based playback.
   */
  bool HasValidTimestamps() const;
  const wxString& GetFileStatus() const { return m_fileStatus; }

private:
  class TimerHandler : public wxTimer {
  public:
    TimerHandler(vdr_pi* plugin) : m_plugin(plugin) {}
    void Notify() { m_plugin->Notify(); }

  private:
    vdr_pi* m_plugin;
  };
  bool LoadConfig(void);
  bool SaveConfig(void);
  wxString FormatNMEA0183AsCSV(const wxString& nmea);
  bool ParseCSVHeader(const wxString& header);
  /** Parse timestamp from a CSV line or raw NMEA sentence. */
  wxString ParseCSVLineTimestamp(const wxString& line, wxDateTime* timestamp);
  /** Return true if the message is a NMEA0183 or AIS message */
  bool IsNMEA0183OrAIS(const wxString& message);
  /** Parse timestamp from NMEA0183 sentence. */
  bool ParseNMEATimestamp(const wxString& nmea, wxDateTime* timestamp);
  /** Update SignalK event listeners when preferences are changed. */
  void UpdateSignalKListeners();
  /** Update NMEA 2000 event listeners when preferences are changed. */
  void UpdateNMEA2000Listeners();
  /** Process incoming NMEA 2000 message from OpenCPN. */
  void OnN2KEvent(wxCommandEvent& ev);
  /** Process incoming SignalK message from OpenCPN. */
  void OnSignalKEvent(wxCommandEvent& ev);

  /**
   * Get the next non-empty line from the input stream. Empty lines are skipped.
   * A line is considered empty if it contains only whitespace.
   *
   * @param fromStart If true, starts reading from the beginning of the file.
   *                 If false, continues from current position.
   * @return The next non-empty line with leading/trailing whitespace removed,
   *         or empty string if no more non-empty lines exist or file isn't
   * open.
   */
  wxString GetNextNonEmptyLine(bool fromStart = false);

  int m_tb_item_id_record;
  int m_tb_item_id_play;

  /** Configuration object for saving/loading settings. */
  wxFileConfig* m_pconfig;
  wxAuiManager* m_pauimgr;
  VDRControl* m_pvdrcontrol;
  /** Input filename for playback. */
  wxString m_ifilename;
  /** Output filename for recording. */
  wxString m_ofilename;
  /** Directory where recordings are saved. */
  wxString m_recording_dir;
  int m_interval;
  /** Flag indicating whether recording is active. */
  bool m_recording;
  /**
   * Flag to track if recording is temporarily paused.
   *
   * This flag is used to pause recording when the boat speed drops below the
   * threshold and then rises above it again.
   */
  bool m_recording_paused;
  /** Start time of the current recording session. */
  wxDateTime m_current_recording_start;
  /** When recording was last paused. */
  wxDateTime m_recording_pause_time;
  /** Flag indicating whether playback is active. */
  bool m_playing;
  /** Flag indicating whether end of file has been reached. */
  bool m_atFileEnd;

  /** Current data format used for recording and playback. */
  VDRDataFormat m_data_format;
  /** Active protocol recording settings. */
  VDRProtocolSettings m_protocols;

  /** Input file stream for playback. */
  wxTextFile m_istream;
  /** Output file stream for recording. */
  wxFile m_ostream;
  /** Plugin toolbar icon. */
  wxBitmap m_panelBitmap;

  /**
   * Active NMEA 2000 message listeners.
   *
   * Each listener monitors specific PGN messages from the network.
   */
  std::vector<std::shared_ptr<ObservableListener>> m_n2k_listeners;
  /**
   * Active Signal K data listeners.
   *
   * Each listener monitors specific Signal K data paths.
   */
  std::vector<std::shared_ptr<ObservableListener>> m_signalk_listeners;

  /** Flag indicating if current file is CSV format. */
  bool m_is_csv_file;
  /** Column headers when reading CSV format files. */
  wxArrayString m_header_fields;
  /**
   * Index of timestamp column in CSV format.
   *
   * Set to -1 if timestamp column not found.
   */
  unsigned int m_timestamp_idx;
  /**
   * Index of NMEA message column in CSV format.
   *
   * Set to -1 if message column not found.
   */
  unsigned int m_message_idx;

  /** Whether to automatically rotate log files. */
  bool m_log_rotate;
  /** Log rotation interval in hours. */
  int m_log_rotate_interval;
  /** When current recording started. */
  wxDateTime m_recording_start;
  /**  Real time when playback started. */
  wxDateTime m_playback_base_time;

  /** The first (earliest) timestamp in the VDR file. */
  wxDateTime m_firstTimestamp;
  /** The last timestamp in the VDR file. */
  wxDateTime m_lastTimestamp;
  /** The current timestamp during VDR playback. */
  wxDateTime m_currentTimestamp;
  /** Track whether file has valid timestamps. */
  bool m_has_timestamps;

  /**
   * Configuration parameter to control whether auto-start recording is enabled
   * or not.
   *
   * If enabled, recording will start automatically when the plugin is loaded.
   * Optionally, there may be a speed threshold that must be met before
   * recording starts.
   */
  bool m_auto_start_recording;
  bool m_use_speed_threshold;  //!< Use speed threshold for auto recording.
  double m_speed_threshold;    //!< Speed threshold for auto recording.
  double m_last_speed;         //!< Last known boat speed.
  /**
   * Indicate user has manually disabled recording while auto-recording was in
   * progress.
   *
   * This flag is used to prevent auto-recording from starting again immediately
   * after the user manually stops recording.
   * If the user manually stops recording, auto-recording will only start again
   * if the boat speed drops below the threshold and then rises above it again.
   */
  bool m_recording_manually_disabled;
  int m_stop_delay;  //!< Minutes to wait before stopping.
  /** When speed first dropped below threshold. */
  wxDateTime m_below_threshold_since;
  wxString m_fileStatus;

  wxEvtHandler* m_eventHandler;
  TimerHandler* m_timer;

#ifdef __ANDROID__
  wxString m_temp_outfile;
  wxString m_final_outfile;
#endif
};

/**
 * UI control panel for VDR playback functionality.
 *
 * Provides controls for loading VDR files, starting/pausing playback,
 * adjusting playback speed, and monitoring playback progress.
 */
class VDRControl : public wxWindow {
public:
  /**
   * Create a new VDR control panel.
   *
   * Initializes UI elements and loads any previously configured VDR file.
   * @param pparent Parent window for the control panel
   * @param id Window identifier
   * @param vdr Owner VDR plugin instance
   */
  VDRControl(wxWindow* pparent, wxWindowID id, vdr_pi* vdr);

  /**
   * Update UI elements for color scheme changes.
   * @param cs New color scheme to apply
   */
  void SetColorScheme(PI_ColorScheme cs);

  /**
   * Update progress indication for playback position.
   * @param fraction Current position as fraction between 0-1
   */
  void SetProgress(double fraction);

  /** Update state of UI controls based on playback status. */
  void UpdateControls();

  /**
   * Update displayed filename in UI.
   * @param filename Path of currently loaded file
   */
  void UpdateFileLabel(const wxString& filename);

  /** Update displayed timestamp in UI based on current playback position. */
  void UpdateTimeLabel();

  /** Get current playback speed multiplier setting. */
  double GetSpeedMultiplier() const { return m_speedSlider->GetValue(); }

private:
  /** Create and layout UI controls. */
  void CreateControls();

  /**
   * Handle file load button clicks.
   *
   * Shows file selection dialog and loads selected VDR file.
   */
  void OnLoadButton(wxCommandEvent& event);

  /**
   * Handle play/pause button clicks.
   *
   * Toggles between playback and paused states.
   */
  void OnPlayPauseButton(wxCommandEvent& event);

  /**
   * Handle playback speed adjustment.
   *
   * Updates playback timing when speed multiplier changes.
   */
  void OnSpeedSliderUpdated(wxCommandEvent& event);

  /**
   * Handle progress slider dragging.
   *
   * Temporarily pauses playback while user drags position slider.
   */
  void OnProgressSliderUpdated(wxScrollEvent& even);

  /**
   * Handle progress slider release.
   *
   * Seeks to new position and resumes playback if previously playing.
   */
  void OnProgressSliderEndDrag(wxScrollEvent& event);

  /** Handle data format selection changes. */
  void OnDataFormatRadioButton(wxCommandEvent& event);

  wxButton* m_loadBtn;         //!< Button to load VDR file
  wxButton* m_playPauseBtn;    //!< Toggle button for play/pause
  wxString m_playBtnTooltip;   //!< Tooltip text for play state
  wxString m_pauseBtnTooltip;  //!< Tooltip text for pause state
  wxString m_stopBtnTooltip;   //!< Tooltip text for stop state

  wxSlider* m_speedSlider;     //!< Slider control for playback speed
  wxSlider* m_progressSlider;  //!< Slider control for playback position
  wxStaticText* m_fileLabel;   //!< Label showing current filename
  wxStaticText* m_timeLabel;   //!< Label showing current timestamp
  wxStaticText* m_statusLabel; //!< Label showing info/error message
  vdr_pi* m_pvdr;              //!< Owner plugin instance

  bool m_isDragging;            //!< Flag indicating progress slider drag
  bool m_wasPlayingBeforeDrag;  //!< Playback state before drag started

  DECLARE_EVENT_TABLE()
};

#endif
