//++
// Messages.cpp - implementation of CBadDog object and miscellaneous methods
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
//   This file contains the implementation of the CBadDogs class, which is used
// to log dog related error messages to a CSV file.  It also contains a few
// simple methods to assist with error messages (e.g. Print()) ...
//
//                                              Bob Armstrong [15-Jul-2019]
//
// REVISION HISTORY:
// 15-Jul-19  RLA   New file.
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <stdarg.h>             // va_start(), va_end(), et al ...
#include <assert.h>             // assert() (what else??)
#include <iostream>             // std::ios, std::istream, std::cout
#include "Messages.hpp"         // declarations for this module ...
#include "CSVRow.hpp"           // CSV file row object
#include "CSVFile.hpp"          // a collection of spreadsheet rows
#include "Dog.hpp"              // dog data definitions


// Initialize all the static members of CBadDogs ...
CBadDogs *CBadDogs::m_pBadDogs = NULL;
string    CBadDogs::m_sFileName("");
const string CBadDogs::m_sColumnHeaders("Name,Number,Contact Member,Error");


CBadDogs::CBadDogs(const string sFileName) : CCSVFile()
{
  //++
  //   CBadDogs is a singleton class, so only one instance is ever allowed to
  // exist.  We save a pointer to this instance in the static member m_pBadDogs
  // so that the AddError() procedure can use it to find that instance.  We 
  // also save the log file name, which will be used by the destructor to write
  // out all the stored errors.
  //--
  assert(m_pBadDogs == NULL);
  assert(!sFileName.empty());
  m_pBadDogs = this;  m_sFileName = sFileName;
}


CBadDogs::~CBadDogs()
{
  //++
  //   The destructor writes the log to the file name saved by the constructor
  // and then forgets about this instance ..
  //--
  assert(m_pBadDogs != NULL);
  WriteFile();
}


/*static*/ string CBadDogs::Print (const char *pszFormat, ...)
{
  //++
  //   Convert a printf() style argument list to text and return the string.
  // Seems like the std::string class should have this function already, but
  // it doesn't!
  //--
  char sz[1024];  va_list args;
  memset(sz, 0, sizeof(sz));
  va_start(args, pszFormat);
  vsprintf_s(sz, sizeof(sz), pszFormat, args);
  va_end(args);
  return string(sz);
}


void CBadDogs::AddError (const CDog *pDog, const string sMsg)
{
  //++
  // Add a new bad dog error to this collection ...
  //--
  assert(pDog != NULL);
  MSGS("dog " << pDog->GetName() << " #" << pDog->GetNumber() << " contact " << pDog->GetResponsiblePerson() << " - " << sMsg);
  CCSVRow row(TOTAL_COLUMNS);
  row[COL_DOG_NAME-1]        = pDog->GetName();
  row[COL_DOG_NUMBER-1]      = std::to_string(pDog->GetNumber());
  row[COL_CONTACT_MEMBER-1]  = pDog->GetResponsiblePerson();
  row[COL_MESSAGE-1]         = sMsg;
  CCSVFile::AddRow(row);
}


void CBadDogs::WriteFile (const string &sFileName) const
{
  //++
  //   Write this collection of bad dogs to a CSV file.  If no file name is
  // specified, then use the one passed to the constructor ...
  //--
  string sFN = sFileName.empty() ? m_sFileName : sFileName;
  size_t nDogs = CCSVFile::Write(sFN, m_sColumnHeaders);
  MSGS("Wrote " << nDogs << " bad dogs to " << sFN);
}
