# VDR Plugin Test Plan

## Basic Recording

Disable auto-recording in preferences.

- [ ] Manual start/stop recording via toolbar button.
- [ ] Verify recorded files contain correct data format (raw NMEA or CSV).
- [ ] Verify timestamp format in CSV output is in ISO 8601 format with UTC.
- [ ] Check file rotation works at specified intervals.
- [ ] Verify recording directory is created if doesn't exist.

## Auto-Recording

Enable auto-recording in preferences.

- [ ] Verify recording auto-starts on plugin initialization.
- [ ] Speed threshold start/stop.
    - Start recording when speed exceeds threshold.
    - Stop recording after delay when speed drops below threshold.
    - Check hysteresis prevents rapid start/stop.
    - Start recording again when speed exceeds threshold.
    - Verify recording does not stop when no NMEA data is received. However nothing is written to the VDR file.
- [ ] Verify manual stop disables auto-recording until speed drops below threshold.

## Playback

- [ ] Load and play NMEA file.
- [ ] Load and play CSV file.
- [ ] Pause/resume functionality.
- [ ] Speed control (verify different playback speeds).
- [ ] Progress bar navigation
    - Drag while playing
    - Drag while paused
- [ ] Verify correct timestamp display.
- [ ] End-of-file behavior
    - Button changes to stop icon
    - Restart from beginning works

## Protocol Support

- [ ] NMEA 0183 sentences recorded correctly
- [ ] AIS messages recorded correctly
- [ ] NMEA 2000 PGNs recorded correctly
    - Basic PGNs (like position, heading)
    - Proprietary messages

## Settings

- [ ] Save/load all preferences
- [ ] Protocol enable/disable works
- [ ] Directory selection persists
- [ ] Format selection (NMEA/CSV) works
