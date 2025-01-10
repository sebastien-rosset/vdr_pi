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

#ifndef _VDR_PI_TIME_H_
#define _VDR_PI_TIME_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/datetime.h>
#include <unordered_map>
#include <functional>

struct NMEATimeInfo {
  bool hasDate;  // Whether date information is available
  bool hasTime;  // Whether time information is available
  struct tm tm;
  int millisecond;

  NMEATimeInfo() : hasDate(false), hasTime(false), tm{0}, millisecond(0) {}

  bool IsComplete() const { return hasDate && hasTime; }
};

/**
 * Represents a unique source of time information from NMEA sentences or CSV
 * entry. This is used to track the time source for each NMEA sentence type.
 */
struct TimeSource {
  wxString talkerId;    // GP, GN, etc.
  wxString sentenceId;  // RMC, ZDA, etc.
  int precision;        // Millisecond precision (0, 1, 2, or 3 digits)

  bool operator==(const TimeSource& other) const {
    return talkerId == other.talkerId && sentenceId == other.sentenceId &&
           precision == other.precision;
  }
};

/**
 * Parses NMEA 0183 timestamps from various sentence types.
 */
class TimestampParser {
public:
  TimestampParser()
      : m_lastValidYear(0), m_lastValidMonth(0), m_lastValidDay(0) {}
  /**
   * Parse a timestamp from a NMEA 0183 sentence.
   *
   * This method supports parsing timestamps from RMC, ZDA, and other sentence
   * types.
   *
   * @param sentence NMEA 0183 sentence to parse.
   * @param timestamp Output timestamp.
   * @param precision Output millisecond precision.
   * @return True if sentence contains a timestamp and that timestamp was
   * successfully parsed. The time is returned in UTC.
   */
  bool ParseTimestamp(const wxString& sentence, wxDateTime& timestamp,
                      int& precision);

  /**
   * Parse a timestamp from an ISO 8601 formatted string in UTC format.
   *
   * @param timeStr ISO 8601 timestamp string.
   * @param timestamp Output timestamp in UTC.
   * @return True if the timestamp was successfully parsed.
   */
  bool ParseIso8601Timestamp(const wxString& timeStr, wxDateTime* timestamp);

  // Reset the cached date state
  void Reset();

  /** Parses HHMMSS or HHMMSS.sss format. */
  bool ParseTimeField(const wxString& timeStr, NMEATimeInfo& info,
                      int& precision) const;

  /** Set the desired primary time source. */
  void SetPrimaryTimeSource(const wxString& talkerId, const wxString& msgType,
                            int precision);
  /** Disable the desired primary time source, parse all sentences containing
   * timestamps. */
  void DisablePrimaryTimeSource();

  /**
   * Parse a timestamp from a CSV line.
   *
   * @param line CSV line to parse.
   * @param timestamp_idx Index of the timestamp field.
   * @param message_idx Index of the message field.
   * @param timestamp Output timestamp.
   * @return The message field.
   */
  wxString ParseCSVLineTimestamp(const wxString& line,
                                 unsigned int timestamp_idx,
                                 unsigned int message_idx,
                                 wxDateTime* timestamp);

private:
  // Cache the last valid date seen from NMEA sentences (RMC, ZDA...)
  int m_lastValidYear;
  int m_lastValidMonth;
  int m_lastValidDay;

  /**
   * When true, timestamps are parsed only if they match the primary time source
   * (talker ID, message type and time precision).
   */
  bool m_useOnlyPrimarySource{false};
  /** Primary time source used when m_useOnlyPrimarySource is true. */
  TimeSource m_primarySource;

  // Parses DDMMYY format (used by RMC)
  bool ParseRMCDate(const wxString& dateStr, NMEATimeInfo& info);

  // Validates date components and sets hasDate flag
  bool ValidateAndSetDate(NMEATimeInfo& info);

  // Applies cached date if available
  void ApplyCachedDate(NMEATimeInfo& info) const;
};

/**
 * Represents the details of a time source, including start and end times.
 */
struct TimeSourceDetails {
  wxDateTime startTime;
  wxDateTime currentTime;
  wxDateTime endTime;
  /** Whether the time source is chronological or not. */
  bool isChronological;

  TimeSourceDetails() : isChronological(true) {}
};

/** Custom hash function for TimeSource to use in unordered_map. */
struct TimeSourceHash {
  size_t operator()(const TimeSource& ts) const {
    std::string talkerId(ts.talkerId.ToStdString());
    std::string sentenceId(ts.sentenceId.ToStdString());
    size_t h1 = std::hash<std::string>{}(talkerId);
    size_t h2 = std::hash<std::string>{}(sentenceId);
    size_t h3 = std::hash<int>{}(ts.precision);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

#endif  // _VDR_PI_TIME_H_
