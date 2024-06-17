//++
// Dog.cpp - implementation of CDog object and CDogs collection
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
//   The CDog class encapsulates the data in a single dog record from the NGRR
// database, and the CDogs class is a collection of CDog records that encapsulates
// the entire NGRR database.  CDog contains methods to convert to and from rows
// in the NGRR csv dump, as well as methods to validate individual fields.  The
// latter are especially important since the NGRR web page does absolutely no
// validation on the data entered and you'll find all kinds of interesting and
// creative nonsense in there.
//
//   The CDogs class implements methods to read and write entire NGRR Dog
// Information Report csv files (aka "DIR"s!) and to search for specific dogs
// by microchip, name or NGRR dog number.  The latter is a unique ID number that
// NGRR assigns to each dog.  I've never figured out the exact system where by
// these numbers are generated, but they are guaranteed to be unique.  FWIW,
// microchip numbers are also guaranteed to be unique keys as well (or at least
// they should be!).  Dog names, however, are most definitely not.
//
//                                              Bob Armstrong [8-Jul-2019]
//
// REVISION HISTORY:
//  8-Jul-19  RLA   New file.
// 16-Oct-19  RLA   Now the A/C name appears in the Location field!
// 19-Jan-21  RLA   Update for new DIR file format
//  3-Apr-23  RLA   Update again for yet another DIR file format
// 17-Dec-23  RLA   Allow "none" in the microchip field
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <assert.h>             // assert() (what else??)
#include <string.h>             // strcasecmp() ...
#include <iostream>             // std::ios, std::istream, std::cout
#include <regex>                // regular expression matching ...
#include <chrono>               // date and time arithmetic
#include <locale>               // std::locale, std::toupper
#include <ctype.h>              // old fashioned toupper() and tolower()
#include "Messages.hpp"         // ERRS() macro, et al ...
#include "CSVRow.hpp"           // CSV file row object
#include "CSVFile.hpp"          // a collection of spreadsheet rows
#include "Dog.hpp"              // declarations for this module
#include "Chip.hpp"             // needed for CChip::VerifyMicrochip() ...

// This is the expected header row for the dog information report ...
//   Note that "Micropchip" is misspelled - that's the way it actually is in
// the report generated by the web page.  Don't fix it!!  Moreover, note that
// (for reasons known only to Will!) there are two versions of the report -
// an "old" version and a "new" version.  The only different between the two
// being the order of the Adoption FName/LName and AC FName/LName columns.
// Will didn't bother to fix the spelling errors in the new version, though.
//
// Update - January 2021
//   The Dog Information Report format has changed again, and a column for
// "Dog Breed" has been added after "Dog Sex" and before "Dog Neuter".  So now
// we have the "old old format", the "old new format" and the "new new format"
// to contend with.  Fortunately we no longer process reports in the "old old"
// format, so what was once new has now become old.  And no, the "Micropchip"
// spelling error still isn't fixed...
//
// Update - April 2023
//   Yet another change to the format - a column for "county" has been inserted
// between "Originating Area" and "Adoption Fname".  As before, what was once
// new is now old.  The 2021 format becomes "old" and the 2023 format is "new".
// And yes, "Micropchip" is STILL spelled wrong!
// 
//      "Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex,            Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, AC Fname, AC Lname, Adoption Fname, Adoption Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date
//      "Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex, Dog Breed, Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, AC Fname, AC Lname, Adoption Fname, Adoption Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date
//2020-  Dog Name  Dog Number  Micropchip number  Dog Age  Dog Sex             Dog Neuter  Dog Status  Dog Location  How Acquired  Date Acquired  Primary Contact Fname  Primary Contact Lname  Surrender Fname  Surrender Lname  Surrender Address  Surrender City  Surrender State  Surrender Zip Code  Originating Area  Adoption Fname Adoption Lname AC Fname AC Lname     Adoption Address  Adoption City  Adoption State  Adoption Zip Code  Adoption Area  Adoption Email  Adoption Home Phone  Adoption Work Phone  Adoption Cell Phone  Adoption Status  Adoption or Disposition Date
//2021-  Dog Name  Dog Number  Micropchip number  Dog Age  Dog Sex  Dog Breed  Dog Neuter  Dog Status  Dog Location  How Acquired  Date Acquired  Primary Contact Fname  Primary Contact Lname  Surrender Fname  Surrender Lname  Surrender Address  Surrender City  Surrender State  Surrender Zip Code  Originating Area  Adoption Fname Adoption Lname AC Fname AC Lname	Adoption Address  Adoption City  Adoption State  Adoption Zip Code  Adoption Area  Adoption Email  Adoption Home Phone  Adoption Work Phone  Adoption Cell Phone  Adoption Status  Adoption or Disposition Date
//2023-  Dog Name  Dog Number  Micropchip number  Dog Age  Dog Sex  Dog Breed  Dog Neuter  Dog Status  Dog Location  How Acquired  Date Acquired  Primary Contact Fname  Primary Contact Lname  Surrender Fname  Surrender Lname  Surrender Address  Surrender City  Surrender State  Surrender Zip Code  Originating Area Count  Adoption Fname Adoption Lname AC Fname AC Lname	Adoption Address  Adoption City  Adoption State  Adoption Zip Code  Adoption Area  Adoption Email  Adoption Home Phone  Adoption Work Phone  Adoption Cell Phone  Adoption Status  Adoption or Disposition Date
//const string CDog::m_sOldColumnHeaders("Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex, Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, AC Fname, AC Lname, Adoption Fname, Adoption Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date");
//const string CDog::m_sOldColumnHeaders("Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex, Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, Adoption Fname, Adoption Lname, AC Fname, AC Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date");
const string CDog::m_sOldColumnHeaders("Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex, Dog Breed, Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, Adoption Fname, Adoption Lname, AC Fname, AC Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date");
const string CDog::m_sNewColumnHeaders("Dog Name, Dog Number, Micropchip number, Dog Age, Dog Sex, Dog Breed, Dog Neuter, Dog Status, Dog Location, How Acquired, Date Acquired, Primary Contact Fname, Primary Contact Lname, Surrender Fname, Surrender Lname, Surrender Address, Surrender City, Surrender State, Surrender Zip Code, Originating Area, County, Adoption Fname, Adoption Lname, AC Fname, AC Lname, Adoption Address, Adoption City, Adoption State, Adoption Zip Code, Adoption Area, Adoption Email, Adoption Home Phone, Adoption Work Phone, Adoption Cell Phone, Adoption Status, Adoption or Disposition Date");


/*static*/ string CDog::tolower(const string &str)
{
  //++
  //   Convert a C++ string to lower case and return the result.  Why the C++
  // string class doesn't already have this function is beyond me!  Yes, I know
  // there are a ton of different ways to do this with iterators, for_each() and
  // maybe even transform(), but this works too...
  //--
  string sResult = str;
  for (unsigned i = 0; i < sResult.length(); ++i)
    if (isupper(sResult[i])) sResult[i] = ::tolower(sResult[i]);
  return sResult;
}


void CDog::Initialize (uint32_t nDog)
{
  //++
  //   Initialize this CDog object ...  This sets every field to the null
  // string EXCEPT the dog number, which is always required ...
  //--
  assert(nDog <= MAXDOG);
  m_nNumber = nDog;  m_sName.clear();  m_sMicrochip.clear();  m_sAge.clear();
  m_sSex.clear();  m_sNeuter.clear();  m_sStatus.clear();  m_sLocation.clear();
  m_sHowAcquired.clear();  m_sDateAcquired.clear();  m_sPrimaryContactFName.clear();
  m_sPrimaryContactLName.clear();  m_sSurrenderFName.clear();
  m_sSurrenderLName.clear();  m_sSurrenderAddress.clear();  m_sSurrenderCity.clear();
  m_sSurrenderState.clear();  m_sSurrenderZipCode.clear();  m_sOriginatingArea.clear();
  m_sAdoptionFName.clear();  m_sAdoptionLName.clear();  m_sACFName.clear();
  m_sACLName.clear();  m_sAdoptionAddress.clear();  m_sAdoptionCity.clear();
  m_sAdoptionState.clear();  m_sAdoptionZip.clear();  m_sAdoptionArea.clear();
  m_sAdoptioneMail.clear();  m_sAdoptionHomePhone.clear();  m_sAdoptionWorkPhone.clear();
  m_sAdoptionCellPhone.clear();  m_sAdoptionStatus.clear();  m_sDispositionDate.clear();
  m_fUpdateRequired = false;
}


bool CDog::FromRow (const CCSVRow &row, bool fNew)
{
  //++
  //   This method will extract data from a CSV file row and initialize this dog
  // record.   There are only two ways it can fail - 1) if the row has the wrong
  // number of columns, or 2) if the dog number is invalid.  The dog number is
  // the only field that's validated - for everything else it just accepts
  // whatever garbage happens to be in the database ...
  //--
  assert(row.size() == (fNew ? TOTAL_NEW_COLUMNS : TOTAL_OLD_COLUMNS));
  Initialize();

  //   Now extract the rest of the data.  Remember that the CCSVRow column
  // numbers are zero based, hence the "-1" for every one!
  unsigned nCol = 0;
  m_sName                = row[nCol++];   // dog name
  string sDogNumber      = row[nCol++];   // dog number
  m_sMicrochip           = row[nCol++];   // microchip number
  m_sAge                 = row[nCol++];   // dog's age (when this record was created
  m_sSex                 = row[nCol++];   // Male/Female
  nCol++;                                 // dog breed
  m_sNeuter              = row[nCol++];   // Yes/No
  m_sStatus              = row[nCol++];   // Adopted/Pending/Died/Evaluation/etc
  m_sLocation            = row[nCol++];   // city name
  m_sHowAcquired         = row[nCol++];   // Surrender/Shelter/Craigslist/etc
  m_sDateAcquired        = row[nCol++];   // date
  m_sPrimaryContactFName = row[nCol++];   // usually this is the A/C, but not always
  m_sPrimaryContactLName = row[nCol++];   // ...
  m_sSurrenderFName      = row[nCol++];   // Surrendering party's first name
  m_sSurrenderLName      = row[nCol++];   // ... last name
  m_sSurrenderAddress    = row[nCol++];   // ... street address
  m_sSurrenderCity       = row[nCol++];   // ... city
  m_sSurrenderState      = row[nCol++];   // ... state
  m_sSurrenderZipCode    = row[nCol++];   // ... zip code
  m_sOriginatingArea     = row[nCol++];   // NGRR area associated
  if (fNew) nCol++;                       // county
  m_sAdoptionFName       = row[nCol++];   // Adopting party's first name
  m_sAdoptionLName       = row[nCol++];   // ... last name
  m_sACFName             = row[nCol++];   // area's A/C first name
  m_sACLName             = row[nCol++];   // ... last name
  m_sAdoptionAddress     = row[nCol++];   // Adopting party's street address
  m_sAdoptionCity        = row[nCol++];   // ... city
  m_sAdoptionState       = row[nCol++];   // ... state
  m_sAdoptionZip         = row[nCol++];   // ... zip code
  m_sAdoptionArea        = row[nCol++];   // NGRR area (if the dog was moved)
  m_sAdoptioneMail       = row[nCol++];   // email address
  m_sAdoptionHomePhone   = row[nCol++];   // phone number
  m_sAdoptionWorkPhone   = row[nCol++];   // phone number
  m_sAdoptionCellPhone   = row[nCol++];   // phone number
  m_sAdoptionStatus      = row[nCol++];   // 
  m_sDispositionDate     = row[nCol++];   // date adoption contract was recorded

  // Allow "None" for the microchip field ...
  if (_stricmp(m_sMicrochip.c_str(), "none") == 0) m_sMicrochip.clear();

  // Parse the dog number and make sure it's legal ...
  std::tr1::regex reNumber("\\d+");
  if (!std::tr1::regex_match(sDogNumber, reNumber)) 
    {BADDOGS(this, "invalid dog number " << sDogNumber);  return false;}
  m_nNumber = std::stoi(sDogNumber);
  if ((m_nNumber == 0) || (m_nNumber > MAXDOG))
    {BADDOGS(this, "invalid dog number " << m_nNumber);  return false;}
  return true;
}


void CDog::ToRow (CCSVRow &row, bool fNew) const
{
  //++
  // This method will convert this dog data into a CSV file row ...
  //--
  assert(false);
#ifdef UNUSED
  row[COL_DOG_NUMBER-1] = std::to_string(m_nNumber);
  row[COL_DOG_NAME-1]                     = m_sName;
  row[COL_MICROCHIP_NUMBER-1]             = m_sMicrochip;
  row[COL_DOG_AGE-1]                      = m_sAge;
  row[COL_DOG_SEX-1]                      = m_sSex;
  row[COL_DOG_NEUTER-1]                   = m_sNeuter;
  row[COL_DOG_STATUS-1]                   = m_sStatus;
  row[COL_DOG_LOCATION-1]                 = m_sLocation;
  row[COL_HOW_ACQUIRED-1]                 = m_sHowAcquired;
  row[COL_DATE_ACQUIRED-1]                = m_sDateAcquired;
  row[COL_PRIMARY_CONTACT_FNAME-1]        = m_sPrimaryContactFName;
  row[COL_PRIMARY_CONTACT_LNAME-1]        = m_sPrimaryContactLName;
  row[COL_SURRENDER_FNAME-1]              = m_sSurrenderFName;
  row[COL_SURRENDER_LNAME-1]              = m_sSurrenderLName;
  row[COL_SURRENDER_ADDRESS-1]            = m_sSurrenderAddress;
  row[COL_SURRENDER_CITY-1]               = m_sSurrenderCity;
  row[COL_SURRENDER_STATE-1]              = m_sSurrenderState;
  row[COL_SURRENDER_ZIP_CODE-1]           = m_sSurrenderZipCode;
  row[COL_ORIGINATING_AREA-1]             = m_sOriginatingArea;
  row[fNew ? COL_NEW_ADOPTION_FNAME-1 : COL_OLD_ADOPTION_FNAME-1] = m_sAdoptionFName;
  row[fNew ? COL_NEW_ADOPTION_LNAME-1 : COL_OLD_ADOPTION_LNAME-1] = m_sAdoptionLName;
  row[fNew ? COL_NEW_AC_FNAME-1       : COL_OLD_AC_FNAME-1]       = m_sACFName;
  row[fNew ? COL_NEW_AC_LNAME-1       : COL_OLD_AC_LNAME-1]       = m_sACLName;
  row[COL_ADOPTION_ADDRESS-1]             = m_sAdoptionAddress;
  row[COL_ADOPTION_CITY-1]                = m_sAdoptionCity;
  row[COL_ADOPTION_STATE-1]               = m_sAdoptionState;
  row[COL_ADOPTION_ZIP_CODE-1]            = m_sAdoptionZip;
  row[COL_ADOPTION_AREA-1]                = m_sAdoptionArea;
  row[COL_ADOPTION_EMAIL-1]               = m_sAdoptioneMail;
  row[COL_ADOPTION_HOME_PHONE-1]          = m_sAdoptionHomePhone;
  row[COL_ADOPTION_WORK_PHONE-1]          = m_sAdoptionWorkPhone;
  row[COL_ADOPTION_CELL_PHONE-1]          = m_sAdoptionCellPhone;
  row[COL_ADOPTION_STATUS-1]              = m_sAdoptionStatus;
  row[COL_ADOPTION_OR_DISPOSITION_DATE-1] = m_sDispositionDate;
#endif
}


void CDog::Display() const
{
  //++
  // Dump all this dog's data on stdout ...
  //--
  std::cout << ">>>>> Data for dog #" << m_nNumber << " <<<<<" << std::endl;
  std::cout << "\t Name               = \"" << m_sName << "\"" << std::endl;
  std::cout << "\t Microchip          = \"" << m_sMicrochip << "\"" << std::endl;
  std::cout << "\t Age                = \"" << m_sAge << "\"" << std::endl;
  std::cout << "\t Sex                = \"" << m_sSex << "\"" << std::endl;
  std::cout << "\t Neuter             = \"" << m_sNeuter << "\"" << std::endl;
  std::cout << "\t Status             = \"" << m_sStatus << "\"" << std::endl;
  std::cout << "\t Location           = \"" << m_sLocation << "\"" << std::endl;
  std::cout << "\t How Acquired       = \"" << m_sHowAcquired << "\"" << std::endl;
  std::cout << "\t Date Acquired      = \"" << m_sDateAcquired << "\"" << std::endl;
  std::cout << "\t A/C Name           = \"" << m_sACFName << " " << m_sACLName << "\"" << std::endl;
  std::cout << "\t Primary Contact    = \"" << m_sPrimaryContactFName << " " << m_sPrimaryContactLName << "\"" << std::endl;
  std::cout << "\t Surrender Name     = \"" << m_sSurrenderFName << " " << m_sSurrenderLName << "\"" << std::endl;
  std::cout << "\t Surrender Address  = \"" << m_sSurrenderAddress << "\"" << std::endl;
  std::cout << "\t Surrender City     = \"" << m_sSurrenderCity << "\"" << std::endl;
  std::cout << "\t Surrender State    = \"" << m_sSurrenderState << "\"" << std::endl;
  std::cout << "\t Surrender Zip      = \"" << m_sSurrenderZipCode << "\"" << std::endl;
  std::cout << "\t Originating Area   = \"" << m_sOriginatingArea << "\"" << std::endl;
  std::cout << "\t Adopter Name       = \"" << m_sAdoptionFName << " " << m_sAdoptionLName << "\"" << std::endl;
  std::cout << "\t Adopter Address    = \"" << m_sAdoptionAddress << "\"" << std::endl;
  std::cout << "\t Adopter City       = \"" << m_sAdoptionCity << "\"" << std::endl;
  std::cout << "\t Adopter State      = \"" << m_sAdoptionState << "\"" << std::endl;
  std::cout << "\t Adopter Zip        = \"" << m_sAdoptionZip << "\"" << std::endl;
  std::cout << "\t Adopter Area       = \"" << m_sAdoptionArea << "\"" << std::endl;
  std::cout << "\t Adopter EMail      = \"" << m_sAdoptioneMail << "\"" << std::endl;
  std::cout << "\t Adopter Home Phone = \"" << m_sAdoptionHomePhone << "\"" << std::endl;
  std::cout << "\t Adopter Work Phone = \"" << m_sAdoptionWorkPhone << "\"" << std::endl;
  std::cout << "\t Adopter Cell Phone = \"" << m_sAdoptionCellPhone << "\"" << std::endl;
  std::cout << "\t Adoption Status    = \"" << m_sAdoptionStatus << "\"" << std::endl;
  std::cout << "\t Disposition Date   = \"" << m_sDispositionDate << "\"" << std::endl;
  std::cout << std::endl;
}


/*static*/ bool CDog::ParseDate(const string &sDate, uint32_t &nDay, uint32_t &nMonth, uint32_t &nYear)
{
  //++
  // Parse a date string in the format YYYY-MM-DD ...
  //--
  std::tr1::regex reDate("^(\\d{4})\\-(\\d{2})\\-(\\d{2})$");
  std::tr1::smatch rmDate;
  if (!std::tr1::regex_search(sDate, rmDate, reDate)) return false;
  assert(rmDate.size() == 4);
  nYear = std::stoi(rmDate.str(1));
  nMonth = std::stoi(rmDate.str(2));
  nDay = std::stoi(rmDate.str(3));
  if ((nMonth == 0) || (nMonth > 12)) return false;
  if ((nDay == 0) || (nDay > 31)) return false;
  if ((nYear < 1990) || (nYear > 2099)) return false;
  return true;
}


/*static*/ string CDog::FormatDate (uint32_t nDay, uint32_t nMonth, uint32_t nYear)
{
  //++
  // Convert a date into the standard "MM/DD/YYYY" format ....
  //--
  char sz[256];
  _snprintf_s(sz, sizeof(sz), "%02d/%02d/%04d", nMonth, nDay, nYear);
  return string(sz);
}


bool CDog::ComputeBirthday (string &sDOB) const
{
  //++
  //   This method will attempt to compute the dog's date of birth.  This is
  // tricky because what the DIR actually gives us is the dog's age, and that's
  // relative to the date the dog was acquired.  Worse, the age is formatted
  // oddly as a string "nn Years nn Months".  We'll have to parse that, parse
  // the date acquired, and then do some math.
  //
  //   The answer will be returned as a string in the format "MM/DD/YYYY".
  // Note that the actual day will always be returned as "01" because the
  // NGRR database only records years and months.  Lastly, if the birthday
  // cannot be determined, because either the acquisition date or the age
  // is invalid, then false is returned along with a null string.
  //--

  // If either the acquisition date or the age is blank, then give up ...
  sDOB.clear();
  if (m_sDateAcquired.empty() || m_sAge.empty()) return false;

  // Parse the age field ...
  std::tr1::regex reAge("^(\\d+)\\syears\\s(\\d+)\\smonths$");
  std::tr1::smatch rmAge;
  string sAge = tolower(m_sAge);
  if (!std::tr1::regex_search(sAge, rmAge, reAge)) return false;
  assert(rmAge.size() == 3);
  uint32_t nAgeYears = std::stoi(rmAge.str(1));
  uint32_t nAgeMonths = std::stoi(rmAge.str(2));
  if ((nAgeMonths > 12) || (nAgeYears > 20)) return false;

  // Parse the date acquired ...
  uint32_t nYearAcquired, nMonthAcquired, nDayAcquired;
  if (!GetDateAcquired(nDayAcquired, nMonthAcquired, nYearAcquired)) return false;

  //   Now compute the actual date of birth from the age and acquisition.
  // Yes, we could probably use std::chrono and time_points or some other such
  // stuff to do this, but that gets really messy.  Turns out it's easier just
  // to do the math directly!
  int32_t nYear = nYearAcquired-nAgeYears;
  int32_t nMonth = nMonthAcquired-nAgeMonths;
  if (nMonth < 1) {--nYear;  nMonth += 12;}
  sDOB = FormatDate(nDayAcquired, nMonth, nYear);
  return true;
}


bool CDog::VerifyPhone (const string &sWhich, string &sPhone, bool fQuiet) const
{
  //++
  //   Verify the syntax of a phone number and convert it to standard format
  // if necessary.  The standard format for a phone number is simply nine
  // decimal digits, e.g. "4085551212", with no punctuation or other special
  // characters.  Note that we don't currently allow international numbers!
  //--

  //   Leave a totally blank phone number alone.  At the same time, handle a
  // few nonsense words that people like to enter ...
  if (sPhone.empty() || (sPhone == "none")) {
    sPhone.clear();   return true;
  }

  //   Amazingly (or maybe not!) I was able to recognize all the common phone
  // number formats, as well as some dubious ones, with a single regex.  if
  // that matches, then we're golden!
  std::tr1::regex rePhone("^\\+?1?\\s?\\(?(\\d\\d\\d)\\)?[\\s\\-/\\*,\\.]*(\\d\\d\\d)[\\s\\-=\\*,\\.]*(\\d\\d\\d\\d)$");
  std::tr1::smatch rmPhone;
  if (!std::tr1::regex_search(sPhone, rmPhone, rePhone)) {
    if (!fQuiet) BADDOGS(this, "invalid " << sWhich << " phone \"" << sPhone << "\"");
    sPhone.clear();  return false;
  }

  // Success - assemble just the nine magic digits and we're done ...
  assert(rmPhone.size() == 4);
  string sArea = rmPhone[1], sPrefix = rmPhone[2], sNumber = rmPhone[3];
//MSGS("parsed \"" << sPhone << "\"\t-> \"" << (sArea+sPrefix+sNumber) << "\"");
  sPhone = sArea + sPrefix + sNumber;
  return true;
}


bool CDog::VerifyZip (string &sZip) const
{
  //++
  //   This method will verify the syntax of a zip code, which must be either
  // five decimal digits, or the "ZIP+4" format of "nnnnn-nnnn".  No other
  // formats are accepted.  Of course this doesn't mean that the zip actually
  // is assigned by the USPS, but at least the syntax is correct.
  //
  //    BTW, in this case a null zip code is NOT acceptable!
  //--
  if (sZip.empty()) 
    {BADDOGS(this, "zip code cannot be blank");  return false;}
  std::tr1::regex reZip("^\\d{5}(\\-\\d{4})?$");
  if (std::tr1::regex_match(sZip, reZip)) return true;
  BADDOGS(this, "invalid zip code \"" << sZip << "\"");
  sZip.clear();  return false;
}


bool CDog::VerifyeMail (string &seMail) const
{
  //++
  //   This method will verify that an email address is syntactically valid.
  // Of course, that doesn't prove that it's a valid email address; only that
  // it LOOKS like a valid address...  Note that this regex is hardly perfect
  // and probably acccepts a few things that it shouldn't, but it's pretty
  // close.  Null email addresses are not allowed!
  //--
  if (seMail.empty())
    {BADDOGS(this, "email address cannot be blank");  return false;}
  std::tr1::regex reeMail("^[[:alnum:]_%\\+\\-\\.]+@[[:alnum:]\\.\\-]+\\.[[:alpha:]]{2,}$");
  if (std::tr1::regex_match(seMail, reeMail)) return true;
  BADDOGS(this,"invalid email address \"" << seMail << "\"");
  seMail.clear();  return false;
}


bool CDog::VerifyState (string &sState) const
{
  //++
  // Verify a legal USPS 2 letter state name abbreviation ...
  //--
  
  //   This is a huge kludge, but there are a fair number of people who omit
  // their state.  If it's blank, assume California!
  if (sState.empty()) {
    //BADDOGS(this, "state cannot be blank");  return false;
    sState = "CA";  return true;
  }

  // Otherwise make sure it's valid ...
  static string sStates = "|AL|AK|AS|AZ|AR|CA|CO|CT|DE|DC|FM|FL|GA|GU|HI|ID|IL|IN|IA|KS|KY|LA|ME|MH|MD|MA|MI|MN|MS|MO|MT|NE|NV|NH|NJ|NM|NY|NC|ND|MP|OH|OK|OR|PW|PA|PR|RI|SC|SD|TN|TX|UT|VT|VI|VA|WA|WV|WI|WY|";
  if ((sState.length() == 2)  &&  (sStates.find(sState) != string::npos)) return true;
  BADDOGS(this,"invalid state \"" << sState << "\"");
  sState.clear();  return false;
}


bool CDog::VerifySex()
{
  //++
  // Verify the sex (Male/Female) of a dog ...
  //--
  string s = tolower(m_sSex);
  if ((s == "female")  ||  (s == "male")) return true;
  BADDOGS(this, "invalid sex \"" << m_sSex << "\"");
  m_sSex = "Male";  return false;
}


bool CDog::VerifySpayNeuter()
{
  //++
  // Verify the spay/neuter status (Yes/No) of a dog ...
  //--
  string s = tolower(m_sNeuter);
  if ((s == "yes")  ||  (s == "no")) return true;
  BADDOGS(this, "invalid spay/neuter \"" << m_sNeuter << "\"");
  m_sNeuter = "Yes";  return false;
}


bool CDog::VerifyAll()
{
  //++
  // Verify (and possibly fix) ALL dog data ...
  //--
  bool fOK = true;

  // These tests apply to all dogs, adopted or not ...
  fOK &= VerifySex();
  fOK &= VerifySpayNeuter();

  // Verify the dog's age ...
  string sDOB;
  if (!ComputeBirthday(sDOB))
    {BADDOGS(this, "has no valid DOB");  fOK = false;}

  //   You would think that verifying the dog adoption data would be easy, but
  // no such luck.  If the dog isn't adopted then all the adoption data should
  // be blank and if he is adopted then all the adopter data must be valid, but
  // how to tell if the dog is adopted?  You might think we could just check
  // the status for "Adopted", but that's not always valid.
  //
  //   The only safe scheme I've found is to check the adopter name.  If an
  // adopter name is present, then we require the remaining adopter data to
  // be valid.  If there's no name, then we require the rest to be blank.
  //
  //   There's one more complication - if the dog is adopted by an NGRR
  // volunteer (a "failed foster"!) then the DIR won't contain any adopter
  // information at all!  Instead, this adoption data shows up in the micro-
  // chip report.  We're OK in that case, though, because the CChips object
  // will copy the adopter data to this dog record when the chip file is
  // loaded.  That case we don't have to worry about.
  if (!m_sAdoptionFName.empty() | !m_sAdoptionLName.empty()) {
    // An email address is required!
    fOK &= VerifyAdoptioneMail();
    // A home phone is required, but the cell and work are optional ...
    fOK &= VerifyHomePhone();
    VerifyCellPhone(true);
    VerifyWorkPhone(true);
    //if (m_sAdoptionCellPhone.empty() && m_sAdoptionHomePhone.empty() && m_sAdoptionWorkPhone.empty()) {
    //  BADDOGS(this, "no valid phone number");  fOK = false;
    //}
    // Verify the adopter's mailing address ...
    fOK &= VerifyAdoptionZip();
    fOK &= VerifyAdoptionState();
  } else {
    bool fBlank = m_sAdoptioneMail.empty() & m_sAdoptionFName.empty() & m_sAdoptionLName.empty()
       & m_sAdoptionCellPhone.empty() & m_sAdoptionHomePhone.empty() & m_sAdoptionWorkPhone.empty()
       & m_sAdoptionAddress.empty() & m_sAdoptionState.empty() & m_sAdoptionZip.empty();
    if (!fBlank) BADDOGS(this, "adoption information should be blank");
    fOK &= fBlank;
  }
  return fOK;
}


string CDog::GetResponsiblePerson() const
{
  //++
  //   This routine will attempt to figure out which NGRR person is responsible
  // for this dog.  Normally this would be the A/C, but not all dog records have
  // A/C names recorded.  If there's no A/C, then the primary contact is returned
  // instead.  If there's no primary contact, then the area name (which isn't
  // really a person, but it's the best we can do) is used.
  //--

  // After much discussion, the primary contact is now preferred over the A/C!
  if (!m_sPrimaryContactFName.empty() || !m_sPrimaryContactLName.empty())
    return m_sPrimaryContactFName + " " + m_sPrimaryContactLName;
  if (!m_sACFName.empty() || !m_sACLName.empty())
    return m_sACFName + " " + m_sACLName;
  if (!m_sLocation.empty()) return m_sLocation;
  return m_sOriginatingArea;
}


bool CDog::WasAcquiredAfter (uint32_t nYear) const
{
  //++
  // Check the dog's acquisition date and verify that it is after nYear ...
  //--
  uint32_t nYearAcquired, nMonthAcquired, nDayAcquired;
  if (!GetDateAcquired(nDayAcquired, nMonthAcquired, nYearAcquired)) {
    BADDOGS(this, "no acquisition date recorded");  return true;
  } else {
    return (nYearAcquired >= nYear);
  }
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


void CDogs::DeleteAll()
{
  //++
  //   Delete all the CDog objects in this collection ...  Note that it's
  // sufficient to delete all the CDog objects contained in the NGRR number
  // map alone, since we know that every dog in the microchip map is also
  // contained in both.
  //--
  for (dog_number_iterator it = dog_begin(); it != dog_end(); ++it)  delete it->second;
  m_mapNumber.clear();  m_mapChip.clear();
}


bool CDogs::Add (CDog *pDog)
{
  //++
  //   Add a dog to this collection ...   Note that the NGRR dog number MUST
  // be valid, however the microchip number is optional.  If the latter is
  // not present, then the dog isn't added to the m_mapChip collection (and
  // consequently can't be found by searching for a microchip).
  //--
  assert(pDog != NULL);
  uint32_t nDog = pDog->GetNumber();
  string sChip = pDog->GetChip();

  // First, be sure that the NGRR dog number is unique ...
  if (Find(nDog) != NULL) 
    {BADDOGS(pDog, "already in collection");  return false;}

  //  Now we need to check and ensure that the microchip number, if any, is
  // also unique.  Sometimes the A/Cs make mistakes and enter the same chip
  // twice, so it's not safe to skip this test.  And also note that we want to
  // perform this test before we add the dog to either map!
  if (!sChip.empty()) {
    const CDog *p = Find(sChip);
    if (p != NULL) {
      BADDOGS(pDog,  "and " << p->GetName() << " #" << p->GetNumber() << " have the same microchip");
      return false;
    }
  }

  // All's well - add this dog to both maps ...
  m_mapNumber.insert({nDog, pDog});
  if (!sChip.empty()) m_mapChip.insert({sChip, pDog});
  return true;
}


bool CDogs::Add (const CCSVRow &row)
{
  //++
  // Create a CDog from a CSV row and add it to this collection ...
  //--
  CDog *pDog = new CDog;
  if (pDog->FromRow(row)) return true;
  delete pDog;  return false;
}


CDog *CDogs::Find (uint32_t nDog) const
{
  //++
  //   Find a dog given it's NGRR dog number and return a pointer to the CDog
  // object, or NULL if none exists.  Remember that just because a dog exists
  // doesn't guarantee that it has a microchip too!
  //--
  dog_number_const_iterator it = m_mapNumber.find(nDog);
  return (it != dog_end())  ?  it->second  :  NULL;
}


CDog *CDogs::Find (string sChip) const
{
  //++
  // Same as above, but this time search the microchip map ...
  //--
  microchip_const_iterator it = m_mapChip.find(sChip);
  return (it != chip_end())  ?  it->second  :  NULL;
}


void CDogs::ReadFile (const string &sFileName, uint32_t nYear, bool fNew)
{
  //++
  //   Read the entire CDogs collection from the Dog Information Report (aka
  // "DIR") CSV file output by the NGRR web page.  One problem with the dog
  // information report is that it contains records for ALL dogs who were ever
  // in NGRR, all the way back to the beginning of time.  There's no way to set
  // an age range on the NGRR web page when generating this report.  The nYear
  // parameter specifies a cutoff year for dogs - any dog with an acquisition
  // date BEFORE nYear will be discarded.  This greatly reduces the amount of
  // data we need to store and process.
  //--
  CCSVFile csv;
  size_t nRows = csv.Read(sFileName, fNew ? CDog::m_sNewColumnHeaders : CDog::m_sOldColumnHeaders);
  MSGS("Read " << nRows << " rows from " << sFileName);
  if (csv.size() == 0) return;

  for (CCSVFile::const_iterator it = csv.begin(); it != csv.end(); ++it) {
    CDog *pDog = new CDog;
    if (pDog->FromRow(**it, fNew)) {
      if (pDog->WasAcquiredAfter(nYear)) Add(pDog);
      //   Note that we don't verify the dog's data here - that's a fool's
      // errand as the NGRR database is full of junk.  We only verify the dog
      // data for dogs with microchips that need registering.
      //pDog->VerifyAll();
    }
  }
  MSGS("CDogs created, " << DogCount() << " dogs, " << ChipCount() << " chips");
}


void CDogs::WriteFile (const string &sFileName, bool fNew) const
{
  //++
  // Write the dog collection back to a CSV file in the same format ...
  //--
  CCSVFile csv;
  for (dog_number_const_iterator it = dog_begin(); it != dog_end(); ++it) {
    const CDog *pDog = it->second;
    CCSVRow row(fNew ? CDog::TOTAL_NEW_COLUMNS : CDog::TOTAL_OLD_COLUMNS);
    pDog->ToRow(row, fNew);  csv.AddRow(row);
  }
  size_t nDogs = csv.Write(sFileName, fNew ? CDog::m_sNewColumnHeaders : CDog::m_sOldColumnHeaders);
  MSGS("Wrote " << nDogs << " rows to " << sFileName);
}


void CDogs::VerifyNewMicrochips (uint32_t nYear) const
{
  //++
  //   This routine will verify that all dogs with an acquisition date AFTER
  // the year specified have a microchip number entered in the database.
  // They're supposed to, but sometimes A/Cs forget ...
  //--
  for (dog_number_const_iterator it = dog_begin(); it != dog_end(); ++it) {
    const CDog *pDog = it->second;
    if (pDog->IsDead() || pDog->IsReturned()) continue;
    if (pDog->WasAcquiredAfter(nYear) && !pDog->HasChip())
        BADDOGS(pDog, "should have a microchip!!");
  }
}
