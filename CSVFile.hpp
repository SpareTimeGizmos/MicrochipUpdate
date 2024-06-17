//++
// CSVFile.hpp - collection of rows (i.e. a spreadsheet file!)
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
//   This class is essentially a collecton of CCSVRow objects.  Since a
// spreadsheet is itself just a collection of rows, this class is thus an
// entire CSV file.  Note that this class OWNS the CCSVRow objects.  Adding
// a row creates a copy of the CCSVRow object, and deleting this object will
// delete all the associated CCSVRow objects!
//
//                                              Bob Armstrong [5-Jul-2019]
//
// REVISION HISTORY:
//  5-JUL-19  RLA   New file.
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#pragma once
#include <string>               // C++ std::string class, et al ...
#include <iostream>             // C++ style output ...
#include <vector>               // C++ vector collection ...
using std::size_t;              // ...
using std::string;              // ...
using std::vector;              // ...
using std::istream;             // ...
using std::ostream;             // ...
class CCSVRow;                  // need some forward references to these 


class CCSVFile {
  //++
  //--

public:
  // Define the row collection vector ...
  typedef vector<CCSVRow *> ROW_VECTOR;
  typedef ROW_VECTOR::iterator iterator;
  typedef ROW_VECTOR::const_iterator const_iterator;

public:
  // Constructors ...
  CCSVFile (const ROW_VECTOR &rows) {CopyRows(rows);}
  CCSVFile() {ClearRows();}
  // Copy and assignment constructors ...
  CCSVFile (const CCSVFile &csv) {CopyRows(csv.m_vecRows);}
  CCSVFile& operator= (const CCSVFile &csv) {ClearRows();  CopyRows(csv.m_vecRows);  return *this;}
  // Destructor ...
  virtual ~CCSVFile() {ClearRows();};

  // CCSVFile collection properties ...
public:
  // Delegate the iterators and array access for the columns ...
  iterator begin() {return m_vecRows.begin();}
  const_iterator begin() const {return m_vecRows.begin();}
  iterator end() {return m_vecRows.end();}
  const_iterator end() const {return m_vecRows.end();}
  // Return the number of columns in this row ...
  size_t size() const {return m_vecRows.size();}
  // Return a reference to column/field "N" ...
  const CCSVRow* operator[] (size_t n) const {return m_vecRows[n];}
  CCSVRow* operator[] (size_t n) {return m_vecRows[n];}

  // CCSVFile public methods ...
public:
  // Empty all the columns ...
  void ClearRows();
  // Copy all the columns ...
  void CopyRows(const ROW_VECTOR &rows);
  void CopyRows(const CCSVFile &csv) { CopyRows(csv.m_vecRows); }
  // Add a row to the end of this spreadsheet ...
  void AddRow(const CCSVRow &row);
  // Add a collection of rows to this spreadsheet ...
  void AddRows(const ROW_VECTOR &rows);
  void AddRows(const CCSVFile &csv) { AddRows(csv.m_vecRows); }
  // Read this spreadsheet from a file ...
  size_t Read (istream &stm, const string sHeader="");
  size_t Read (const string &sFileName, const string sHeader="");
  // Write this spreadsheet to a file ...
  size_t Write (ostream &stm, const string sHeader="") const;
  size_t Write (const string &sFileName, const string sHeader="") const;

  // Private internal CCSVFile methods ...
protected:

  // Local CCSVFile members ...
protected:
  ROW_VECTOR m_vecRows;         // rows in this spreadsheet
};
