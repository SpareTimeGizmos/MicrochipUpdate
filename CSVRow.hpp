//++
// CSVRow.hpp -> class for single rows of a spreadsheet
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
//   This class represents one line or row of a CSV file.  It's a collection
// of strings, each one representing one field or column.  The nice thing
// about this collection is that it knows how to read or write itself from
// or to an iostream...
//
//                                              Bob Armstrong [4-Jul-2019]
//
// REVISION HISTORY:
//  4-JUL-19  RLA   New file.
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


class CCSVRow
{
  //++
  //--

public:
  // Magic constants ...
  enum {
    COMMA = ',',                // delimiter used between fields in CSV files
    QUOTE = '"'                 // character used to quote literal strings
  };

public:
  // Define the column collection vector ...
  typedef vector<string> COLUMN_VECTOR;
  typedef COLUMN_VECTOR::iterator iterator;
  typedef COLUMN_VECTOR::const_iterator const_iterator;

public:
  // Constructors ...
  CCSVRow (const COLUMN_VECTOR &cols) {CopyColumns(cols);}
  CCSVRow (const string &str) {Parse(str);}
  CCSVRow (size_t nCols) : m_vecColumns(nCols) {}
  CCSVRow() {ClearColumns();}
  // Copy and assignment constructors ...
  CCSVRow (const CCSVRow &row) {CopyColumns(row);}
  CCSVRow& operator= (const CCSVRow &row) {ClearColumns();  CopyColumns(row);  return *this;}
  // Destructor ...
  virtual ~CCSVRow() {};

  // CCSVRow collection properties ...
public:
  // Delegate the iterators and array access for the columns ...
  iterator begin() {return m_vecColumns.begin();}
  const_iterator begin() const {return m_vecColumns.begin();}
  iterator end() {return m_vecColumns.end();}
  const_iterator end() const {return m_vecColumns.end();}
  // Return the number of columns in this row ...
  size_t size() const {return m_vecColumns.size();}
  // Return a reference to column/field "N" ...
  const string& operator[] (size_t n) const {return m_vecColumns[n];}
  string& operator[] (size_t n) {return m_vecColumns[n];}
  // Compare two rows for equality ...
  const bool operator== (const CCSVRow &row) const {return Verify(row);}

  // CCSVRow public methods ...
public:
  // Parse a string and extract the fields/columns ...
  size_t Parse (const string &str);
  // Read the next line/row from the stream and extract the columns ...
  size_t Read (istream &stm);
  // Verify the column headers ...
  bool Verify (const string &str) const;
  bool Verify (const CCSVRow &row) const;
  // Format the columns into a single line/row ...
  string Format() const;
  // Write this line/row to a stream ...
  void Write (ostream &stm) const  {stm << Format() << std::endl;}

  // Private internal CCSVRow methods ...
protected:
  // Empty all the columns ...
  void ClearColumns() {m_vecColumns.clear();}
  // Copy all the columns ...
  void CopyColumns (const COLUMN_VECTOR &cols) {m_vecColumns = cols;}
  void CopyColumns (const CCSVRow &row) {CopyColumns(row.m_vecColumns);}
  // Add a column to the end of this collection ...
  void AddColumn (const string &str)  {m_vecColumns.push_back(str);}
  // Trim leading and trailing white space from a field ...
  static string TrimColumn (const string &src);
  // Remove the "="..."" gaarbage ...
  static string RemoveEquals (const string &src);
  // Return TRUE if a field contains a quote or comma itself ...
  static bool NeedsQuotes (const string &str);
  // Parse a single field from a string ...
  static string ParseField (const string &str, string::const_iterator &it);
  // Format a single column/field into a string ...
  static string FormatField (const string &col);

  // Local CCSVRow members ...
protected:
  COLUMN_VECTOR m_vecColumns;   // fields/columns in this row
};

// Allow streaming directly to and from CCSVRow objects ...
inline istream& operator>> (istream &stm, CCSVRow &row) {row.Read(stm);  return stm;}
inline ostream& operator<< (ostream &stm, const CCSVRow &row) {row.Write(stm);  return stm;}
