/***************************************************************************
 *   Copyright (C) 2024 by OpenCPN development team                        *
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
 **************************************************************************/

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // precompiled headers

#include "wx/tokenzr.h"

#include "vdr_pi_time.h"

bool TimestampParser::ParseTimeField(const wxString& timeStr,
                                     NMEATimeInfo& info, int* precision) const {
  if (timeStr.length() < 6) return false;

  // Parse base time components
  info.hour = wxAtoi(timeStr.Mid(0, 2));
  info.minute = wxAtoi(timeStr.Mid(2, 2));
  info.second = wxAtoi(timeStr.Mid(4, 2));

  // Parse optional milliseconds
  info.millisecond = 0;
  if (precision) *precision = 0;  // Default precision
  if (timeStr.length() > 7) {     // Has decimal point and subseconds
    // Check if we actually have a decimal point
    if (timeStr[6] != '.') return false;

    // Get the subseconds string
    wxString subsecStr = timeStr.Mid(7);
    if (subsecStr.empty()) return false;

    // Calculate precision from subseconds length
    if (precision) *precision = subsecStr.length();

    // Convert to milliseconds
    double subseconds = wxAtof("0." + subsecStr);
    info.millisecond = static_cast<int>(subseconds * 1000);
  }

  // Validate time components
  if (info.hour < 0 || info.hour > 23 || info.minute < 0 || info.minute > 59 ||
      info.second < 0 || info.second > 59 || info.millisecond < 0 ||
      info.millisecond >= 1000) {
    return false;
  }

  info.hasTime = true;
  return true;
}

bool TimestampParser::ParseRMCDate(const wxString& dateStr,
                                   NMEATimeInfo& info) {
  if (dateStr.length() < 6) return false;

  info.day = wxAtoi(dateStr.Mid(0, 2));
  info.month = wxAtoi(dateStr.Mid(2, 2));
  int twoDigitYear = wxAtoi(dateStr.Mid(4, 2));
  // Use sliding window: years 00-69 are 2000-2069, years 70-99 are 1970-1999
  info.year = (twoDigitYear >= 70) ? 1900 + twoDigitYear : 2000 + twoDigitYear;

  return ValidateAndSetDate(info);
}

bool TimestampParser::ValidateAndSetDate(NMEATimeInfo& info) {
  if (info.month < 1 || info.month > 12 || info.day < 1 || info.day > 31 ||
      info.year < 1900 || info.year > 2200) {
    return false;
  }

  // Cache valid date components for sentences with only time
  m_lastValidYear = info.year;
  m_lastValidMonth = info.month;
  m_lastValidDay = info.day;

  info.hasDate = true;
  return true;
}

// Applies cached date if available
void TimestampParser::ApplyCachedDate(NMEATimeInfo& info) const {
  if (m_lastValidYear > 0) {
    info.year = m_lastValidYear;
    info.month = m_lastValidMonth;
    info.day = m_lastValidDay;
    info.hasDate = true;
  }
}

bool TimestampParser::ParseIso8601Timestamp(const wxString& timeStr,
                                            wxDateTime* timestamp) {
  wxString::const_iterator end;
  wxDateTime tempTime;

  // Try formats with milliseconds
  if (tempTime.ParseFormat(timeStr, "%Y-%m-%dT%H:%M:%S.%l%z", &end)) {
    *timestamp = tempTime;
    return true;
  }

  // Try formats without milliseconds
  if (tempTime.ParseFormat(timeStr, "%Y-%m-%dT%H:%M:%S%z", &end)) {
    *timestamp = tempTime;
    return true;
  }

  return false;
}

bool TimestampParser::ParseTimestamp(const wxString& sentence,
                                     wxDateTime* timestamp, int* precision) {
  // Check for valid NMEA sentence
  if (sentence.IsEmpty() || sentence[0] != '$') {
    return false;
  }

  // Split the sentence into fields
  wxStringTokenizer tok(sentence, wxT(",*"));
  if (!tok.HasMoreTokens()) return false;

  wxString sentenceId = tok.GetNextToken();
  wxString talkerId = sentenceId.Mid(1, 2);
  wxString sentenceType = sentenceId.Mid(3);

  if (m_useOnlyPrimarySource && (m_primarySource.talkerId != talkerId ||
                                 m_primarySource.sentenceId != sentenceType)) {
    return false;
  }
  NMEATimeInfo timeInfo;

  // Handle different sentence types
  if (sentenceType == "RMC") {  // GPRMC, GNRMC, etc
    // Time field
    if (!tok.HasMoreTokens()) return false;
    wxString timeStr = tok.GetNextToken();
    if (!ParseTimeField(timeStr, timeInfo, precision)) return false;

    // Skip to date field (field 9)
    for (int i = 0; i < 7 && tok.HasMoreTokens(); i++) {
      tok.GetNextToken();
    }

    // Parse date
    if (!tok.HasMoreTokens()) return false;
    wxString dateStr = tok.GetNextToken();
    if (!ParseRMCDate(dateStr, timeInfo)) return false;
  } else if (sentenceType == "ZDA") {  // GPZDA, GNZDA, etc
    // Parse time
    if (!tok.HasMoreTokens()) return false;
    wxString timeStr = tok.GetNextToken();
    if (!ParseTimeField(timeStr, timeInfo, precision)) return false;

    // Parse date components
    if (!tok.HasMoreTokens()) return false;
    timeInfo.day = wxAtoi(tok.GetNextToken());
    if (!tok.HasMoreTokens()) return false;
    timeInfo.month = wxAtoi(tok.GetNextToken());
    if (!tok.HasMoreTokens()) return false;
    timeInfo.year = wxAtoi(tok.GetNextToken());

    if (!ValidateAndSetDate(timeInfo)) return false;
  } else if (sentenceType == "GLL") {
    // For GLL, time is in field 5
    for (int i = 0; i < 4 && tok.HasMoreTokens(); i++) {
      tok.GetNextToken();  // Skip lat/lon fields
    }
    if (!tok.HasMoreTokens()) return false;
    wxString timeStr = tok.GetNextToken();
    if (!ParseTimeField(timeStr, timeInfo, precision)) return false;

    // Try to use cached date information
    ApplyCachedDate(timeInfo);
  } else if (sentenceType == "GGA" || sentenceType == "GBS") {
    // These sentences have time in field 1
    if (!tok.HasMoreTokens()) return false;
    wxString timeStr = tok.GetNextToken();
    if (!ParseTimeField(timeStr, timeInfo, precision)) return false;

    // Try to use cached date information
    ApplyCachedDate(timeInfo);
  }
  if (m_useOnlyPrimarySource && *precision != m_primarySource.precision) {
    return false;
  }

  // Return false if we don't have both date and time
  if (!timeInfo.IsComplete()) {
    return false;
  }

  // Create timestamp
  timestamp->Set(timeInfo.day,
                 static_cast<wxDateTime::Month>(timeInfo.month - 1),
                 timeInfo.year, timeInfo.hour, timeInfo.minute, timeInfo.second,
                 timeInfo.millisecond);

  return timestamp->IsValid();
}

void TimestampParser::SetPrimaryTimeSource(const wxString& talkerId,
                                           const wxString& msgType,
                                           int precision) {
  m_primarySource = TimeSource{talkerId, msgType, precision};
  m_useOnlyPrimarySource = true;
}

void TimestampParser::DisablePrimaryTimeSource() {
  m_useOnlyPrimarySource = false;
}

void TimestampParser::Reset() {
  m_lastValidYear = 0;
  m_lastValidMonth = 0;
  m_lastValidDay = 0;
  m_useOnlyPrimarySource = false;
}

wxString TimestampParser::ParseCSVLineTimestamp(const wxString& line,
                                                unsigned int timestamp_idx,
                                                unsigned int message_idx,
                                                wxDateTime* timestamp) {
  wxArrayString fields;
  wxString currentField;
  bool inQuotes = false;

  for (size_t i = 0; i < line.Length(); i++) {
    wxChar ch = line[i];

    if (ch == '"') {
      if (inQuotes && i + 1 < line.Length() && line[i + 1] == '"') {
        // Double quotes inside quoted field = escaped quote
        currentField += '"';
        i++;  // Skip next quote
      } else {
        // Toggle quote state
        inQuotes = !inQuotes;
      }
    } else if (ch == ',' && !inQuotes) {
      // End of field
      fields.Add(currentField);
      currentField.Clear();
    } else {
      currentField += ch;
    }
  }

  // Add the last field
  fields.Add(currentField);

  // Parse timestamp if requested and available
  if (timestamp && timestamp_idx != static_cast<unsigned int>(-1) &&
      timestamp_idx < fields.GetCount()) {
    if (!ParseIso8601Timestamp(fields[timestamp_idx], timestamp)) {
      return wxEmptyString;
    }
  }

  // Get message field
  if (message_idx == static_cast<unsigned int>(-1) ||
      message_idx >= fields.GetCount()) {
    return wxEmptyString;
  }

  // No need to unescape quotes here as we handled them during parsing
  return fields[message_idx];
}