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

#include <gtest/gtest.h>
#include "vdr_pi_time.h"
#include "vdr_pi.h"

TEST(VDRPluginTests, PluginCreation) {
  vdr_pi plugin(nullptr);
  EXPECT_TRUE(true);  // Just a sanity check.
}

TEST(VDRPluginTests, ScanFileTimestamps) {
  vdr_pi plugin(nullptr);

  wxString testdataPath(TESTDATA);
  std::cout << "TESTDATA path: " << testdataPath << std::endl;
  // Test loading a valid file
  wxString testfile = wxString(TESTDATA) + wxString("/hakan.txt");
  ASSERT_TRUE(plugin.LoadFile(testfile)) << "Failed to load test file";

  // Test scanning timestamps
  EXPECT_TRUE(plugin.ScanFileTimestamps()) << "Failed to scan timestamps";

  // Create expected timestamps
  // First line:
  // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A Last
  // line:  $GPRMC,123522,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6F
  wxDateTime expectedFirst;
  expectedFirst.ParseDateTime("1994-03-23 12:35:19");
  wxDateTime expectedLast;
  expectedLast.ParseDateTime("1994-03-23 12:35:22");

  // Verify the timestamps
  EXPECT_TRUE(plugin.GetFirstTimestamp().IsValid())
      << "First timestamp not valid";
  EXPECT_TRUE(plugin.GetLastTimestamp().IsValid())
      << "Last timestamp not valid";

  EXPECT_EQ(plugin.GetFirstTimestamp(), expectedFirst)
      << "First timestamp mismatch";
  EXPECT_EQ(plugin.GetLastTimestamp(), expectedLast)
      << "Last timestamp mismatch";

  // Test with invalid file
  wxString error;
  EXPECT_FALSE(plugin.LoadFile("nonexistent.txt", &error))
      << "Should fail with non-existent file";
  EXPECT_FALSE(plugin.ScanFileTimestamps())
      << "Should fail when no file is loaded";
}