//++
// CSVFile.cpp - implementation of a CCSVRow collection
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
//   The CCSVFile class is a collecton of CCSVRow objects representing an entire
// spreadsheet.  Note that this class OWNS the CCSVRow objects.  Adding a row
// creates a copy of the CCSVRow object, and deleting this object will delete
// all the associated CCSVRow objects!
//
//                                              Bob Armstrong [5-Jul-2019]
//
// REVISION HISTORY:
//  5-Jul-19  RLA   New file.
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <assert.h>             // assert() (what else??)
#include <iostream>             // std::ios, std::istream, std::cout
#include <fstream>              // std::filebuf
#include "Messages.hpp"         // ERRS() macro, et al ...
#include "CSVRow.hpp"           // class for each row of the spreadsheet
#include "CSVFile.hpp"          // declarations for this module



void CCSVFile::ClearRows()
{
  //++
  //  Empty all the rows in this spreadsheet, and delete all the associated
  // CCSVRow objects!
  //--
  for (iterator it = begin();  it != end();  ++it) {
    if (*it != NULL)  delete *it;
    *it = NULL;
  }
}


void CCSVFile::AddRow (const CCSVRow &row)
{
  //++
  //   Add one row to the end of this collection.  Note that this requires
  // making a copy of the CCSVRow object, and we own the copied object from
  // now until ClearRows() is called!
  //--
  CCSVRow *pRow = new CCSVRow(row);
  m_vecRows.push_back(pRow); 
}


void CCSVFile::AddRows (const ROW_VECTOR &rows)
{
  //++
  // Add a collection of rows to this spreadsheet ...
  //--
  for (const_iterator it = rows.begin();  it != rows.end();  ++it) {
    AddRow(**it);
  }
}


void CCSVFile::CopyRows (const ROW_VECTOR &rows)
{
  //++
  //   Delete all the current rows from this spreadsheet, and then copy all
  // the rows from the specified collection ...
  //--
  ClearRows();  AddRows(rows);
}


size_t CCSVFile::Read (istream &stm, const string sHeader)
{
  //++
  //   This method will attempt to read an entire spreadsheet from a CSV file.
  // If strHeader is specified and is not null, then this is expected to match
  // the first row of the CSV as a check that the columns are what we expect
  // them to be, and every subsequent row must have exactly the same number of
  // columns.
  //
  //   If strHeader is omitted or null, then no hreader row is expected and no
  // check is made on the number of columns.  In this case it's possible for
  // different rows in this spreadsheet to have differing column counts!
  //--
  CCSVRow row;  size_t nCols = 0;  uint32_t nLines = 0;

  // If a header was specified, then verify that first ...
  if (!sHeader.empty()) {
    stm >> row;  nCols = row.size();  ++nLines;
    if (!row.Verify(sHeader))
      MSGS("CCSVFile::Read() header does not match");
  }

  // Now read the rest of the file ...
  for (;;) {
    stm >> row;
    if ((stm.eof()) && (row.size() == 0)) break;
    ++nLines;
    if ((nCols > 0)  &&  (row.size() != nCols))
      MSGS("CCSVFile::Read() wrong number of columns in line " << nLines);
    AddRow(row);
  }

  // And we're done ...
  return size();
}


size_t CCSVFile::Read (const string &sFileName, const string sHeader)
{
  //++
  //   This method is similar to the previous one, but it also handles opening
  // the file, reading the spreadsheet, and then closing the file.
  //--
  std::filebuf fb;
  if (!fb.open(sFileName, std::ios::in))
    ERRS("CCSVFile::Read() unable to open " << sFileName);
  std::istream is(&fb);
  Read(is, sHeader);
  fb.close();
  return size();
}


size_t CCSVFile::Write (ostream &stm, const string sHeader) const
{
  //++
  //   This method will write the spreadsheet to a CSV file, including a header
  // row if one is specified in the call.  It just writes out all the rows in
  // the collection; it's pretty simple.
  //--

  // First, write the header (if we have one) ...
  if (!sHeader.empty()) {
    CCSVRow hdr(sHeader); stm << hdr;
  }

  // Then write out all the rows in the collection ...
  if (size() > 0) {
    for (const_iterator it = begin(); it != end(); ++it) stm << **it;
  }
  return size();
}


size_t CCSVFile::Write (const string &sFileName, const string sHeader) const
{
  //++
  // Same as the above, but handle opening and closing the file too ...
  //--
  std::filebuf fb;
  if (!fb.open(sFileName, std::ios::out))
    ERRS("CCSVFile::Write() unable to create " << sFileName);
  std::ostream os(&fb);
  size_t nRows = Write(os, sHeader);
  fb.close();
  return nRows;
}
