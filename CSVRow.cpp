//++
// CSVRow.cpp - implementation of a spreadsheet row object
//
//   COPYRIGHT (C) 2015-2022 BY SPARE TIME GIZMOS.  ALL RIGHTS RESERVED.
//
// LICENSE:
//    This file is part of the NGRR Microchip Data project.  This program is
// free software; you may redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.See the GNU General Public License for
// more details.  You should have received a copy of the GNU General Public
// License along with this program. If not, see https://www.gnu.org/licenses/. 
//
// DESCRIPTION:
//   A row of a shreadsheet is a collection of fields corresponding to
// the columns.  Each field is a string object, so this is really nothing
// more than a collection of strings.  The nice thing about this collection
// is that it knows how to read or write itself from or to an iostream...
//
//                                              Bob Armstrong [4-Jul-2019]
//
// REVISION HISTORY:
//  4-Jul-19  RLA   New file.
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <assert.h>             // assert() (what else??)
#include <iostream>             // std::ios, std::istream, std::cout
#include "Messages.hpp"         // ERRS() macro, et al ...
#include "CSVRow.hpp"           // declarations for this module



string CCSVRow::TrimColumn (const string &src)
{
  //++
  // Trim leading and trailing white space from a field value ...
  //--
  size_t start = src.find_first_not_of(" \t");
  size_t end = src.find_last_not_of(" \t");
  if ((start == string::npos)  ||  (end == string::npos)) return "";
  return src.substr(start, end+1);
}


string CCSVRow::RemoveEquals(const string &src)
{
  //++
  //   Some CSV files force numeric data to be interpreted as a string by 
  // writing something like ="1234" to the CSV file.  All we want is the
  // 1234 without the extra junk, and this method will attempt to remove
  // it.
  //--
  if ((src.length() == 0)  ||  (src[0] != '='))  return src;
  string dst = src.substr(1);
  if (dst.length() < 2) return dst;
  if ((dst[0] != QUOTE)  ||  (dst[dst.length()-1] != QUOTE))  return dst;
  return dst.substr(1, dst.length()-2);
}


bool CCSVRow::NeedsQuotes (const string &str)
{
  //++
  //   Return TRUE if the string contains either a quote or a comma (meaning
  // that it needs to be quoted in the CSV file!) ...
  //--
  if (str.find(QUOTE) != string::npos) return true;
  if (str.find(COMMA) != string::npos) return true;
  return false;
}


string CCSVRow::ParseField (const string &str, string::const_iterator &it)
{
  //++
  //   This routine will scan the next field from the CSV line.  It essentially
  // accumulates all characters up until the next comma or the end of line, BUT
  // we have to be careful to deal with quoted strings along the way ...
  //--
  bool fInQuotes = false, fQuoteLast = false;  string sResult("");
  for (;  it != str.end();  ++it) {
    if ((*it == COMMA)  &&  !fInQuotes)  break;
    if (*it == QUOTE) {
      if (!fInQuotes) {
        if (fQuoteLast) sResult.push_back(QUOTE);
        fQuoteLast = false;  fInQuotes = true;
      } else {
        fInQuotes = false;  fQuoteLast = true;
      }
    } else {
      sResult.push_back(*it);  fQuoteLast = false;
    }
  }
  return sResult;
}


size_t CCSVRow::Parse (const string &str)
{
  //++
  //   This method will parse a row and extract all the columns.  Each column
  // is added to this collection and the total number of columns found will
  // be returned.
  //
  //   NOTE: a null string returns zero columns!  One could argue that a null
  // string should return one column with nothing in it, but that's not the way
  // we handle it!
  //--
  ClearColumns();
  if (str.length() != 0) {
    for (string::const_iterator it = str.begin();; ++it) {
      string sColumn = ParseField(str, it);
      AddColumn(TrimColumn(RemoveEquals(TrimColumn(sColumn))));
      if (it == str.end()) break;
    }
  }
  return size();
}


size_t CCSVRow::Read (istream &stm)
{
  //++
  // Read the next line/row from the stream and extract the columns ...
  //--
  string str;
  if (stm.eof()) return 0;
  std::getline(stm, str);
  return Parse(str);
}


bool CCSVRow::Verify (const CCSVRow &row) const
{
  //++
  //   This method will verify that the columns in the row object passed as
  // a parameter exactly match the columns in this object.   It's primarily
  // used with CSV files that have headers in the first row - we can match
  // the headers to verify that the columns in the CSV are what we expect
  // them to be.
  //
  // It returns TRUE if the columns match exactly and FALSE if they do not.
  //--
  if (size() != row.size()) return false;
  for (size_t i = 0;  i < size();  ++i) {
    if ((*this)[i].compare(row[i]) != 0) return false;
  }
  return true;
}


bool CCSVRow::Verify (const string &str) const
{
  //++
  // Same as above, but with a literal CSV string for the comparison ...
  //--
  CCSVRow row(str);
  return Verify(row);
}


string CCSVRow::FormatField (const string &col)
{
  //++
  //   This method will format a single column/field into a string.  This is
  // trivial UNLESS the field comtains an embedded comma or quote, in which
  // case this field has to be quoted (and any embedded quotes escaped!).
  //--
  if (!NeedsQuotes(col)) return col;
  string str("");    str.push_back(QUOTE);
  for (string::const_iterator it = col.begin();  it != col.end();  ++it) {
    str.push_back(*it);
    if (*it == QUOTE) str.push_back(QUOTE);
  }
  str.push_back(QUOTE);
  return str;
}


string CCSVRow::Format() const
{
  //++
  // Format this collection of columns into a single row ...
  //--
  string sResult("");
  if (size() > 0) {
    for (const_iterator it = begin();  it != end();  ++it) {
      if (it != begin()) sResult.push_back(COMMA);
      sResult.append(FormatField(*it));
    }
  }
  return sResult;
}
