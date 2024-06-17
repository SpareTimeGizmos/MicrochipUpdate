//++
// Chip.cpp - implementation of CChip object and CChips collection
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
// IMPORTANT!
//   Much of this class is pointless now.  It was originally intended that we
// would read the microchip report output by the NGRR web page, but that has
// proved to be mostly useless.  Instead we now derive everything we need to
// know by comparing Dog Information Reports instead.  
//
//   Originally this class was intended just to point at a corresponding dog
// record, under the assumption that the information here would be exactly the
// same as what's there.  That also has proved not to be true, and in many cases
// (e.g. adoption by an NGRR member, dogs which are registered to NGRR and not
// adopted yet, etc) the information we give to Found.org is not the same as
// what's in the dog record.
//
//   It'd be better if this class just encapsulated a Found.org update record
// (call it a "CUpdate" class instead!) and had local copies of all the relevant
// data.  That would allow for what we feed to Found.org to differ from the
// dog data record.
//
//   Next time :)
//
// DESCRIPTION:
//   The CChip class encapsulates the data from the "Dogs Data" (DD) report
// generated by the NGRR web page.  This was originally intended to be the
// file that we would give to Found.org to automate the registration process,
// but the web page has never been able to generate this report in the correct
// format for Found.org.  That's one reason why this program exists - we massage
// the data and then output the CORRECT report.
//
//   As a result, most of the data we read from the dog data CSV is useless.
// All the important information that we really need is already in the CDog
// object, and the dog data CSV basically just gives us a list of dogs that
// need updating.  So when the dog data CSV is read, the first thing we do is
// find the corresponding CDog record and then store a pointer to that in this
// CChip object.  Each new CChip object is added to the CChips collection, and
// then we can iterate thru that and generate a new report for Found.org.
//
//                                            Bob Armstrong [9-Jul-2019]
//
// REVISION HISTORY:
//  9-Jul-19  RLA   New file.
// 17-OCT-19  RLA   Use canned NGRR values for dogs which aren't adopted.
// 28-NOV-22  RLA   Always use today's date as the "Service Date".
// 28-APR-23  RLA   Don't include the dog number in the name anymore!
// 17-DEC-23  RLA   Add a special hack for Pumpkin's 202 chip
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <assert.h>             // assert() (what else??)
#include <iostream>             // std::ios, std::istream, std::cout
#include <regex>                // regular expression matching ...
#include <ctime>                // tine() et al ...
#include "Messages.hpp"         // ERRS() macro, et al ...
#include "CSVRow.hpp"           // CSV file row object
#include "CSVFile.hpp"          // a collection of spreadsheet rows
#include "Dog.hpp"              // dog data declarations
#include "Chip.hpp"             // declarations for this module

// This is the expected header row for the "Dogs Data" (DD) report generated by the NGRR web page ...
const string CChip::m_sNGRRHeaders("Adoption FName,Adoption LName,Email Address,Address 1,Address 2,City,State,Zip Code,Home Phone,Work Phone,Cell Phone,Pet Name,Microchip Number,Service Date,Date of Birth,Species,Sex,Spayed/Neutered,Primary Breed,Secondary Breed,Rescue Group Email,Notes");
const string CChip::m_sFoundHeaders("First Name,Last Name,Email Address,Address 1,Address 2,City,State,Zip Code,Home Phone,Work Phone,Cell Phone,Pet Name,Microchip Number,Service Date,Date of Birth,Species,Sex,Spayed/Neutered,Primary Breed,Secondary Breed,Rescue Group Email,Notes");



/*static*/ bool CChip::VerifyMicrochip (string &sChip, bool fMessage)
{
  //++
  //   Verify (and fix if necessary and possible) the microchip number recorded
  // for this dog.  As with all things microchip, there isn't one standard but
  // several.  Here are the standards and formats that I know about -
  //
  // FDXB or ISO microchips
  //    * All ISO-compliant microchips contain exactly 15 decimal digits only
  //    * If the first digit is a '9', then the first 3 digits or 5 digits of
  //      the code identify the chip manufacturer -
  //        981 -> Datamars
  //        98102 -> Microfindr, Datamars, Bayer resQ, Found Animals, Crystal Tag
  //        98101 -> Banfield
  //        956 -> Trovan, Ltd. (AKC/CAR)
  //        977 -> AVID
  //        982 -> Allflex (24PetWatch.com)
  //        985 -> Destron Fearing/Digital Angel (HomeAgain.com)
  //    * If the first digit is not a 9, then the first two digits represent the
  //      country code of the microchip.
  //
  // FDXA or non-ISO microchips
  //    * Can be either 9-digit numeric or 10-digit alphanumeric codes
  //    * AVID uses a 10 digit, alphanumeric code begining with the number 1 and
  //      ending with the letter A.  These codes contain no other letters.
  //    *  24PetWatch (Allflex) uses a 10 digit alphanumeric code usually beginning with "0A"
  //    * HomeAgain (Destron Fearing/Digital Angel) uses a 10-digit alphanumeric code
  //    * AKC/CAR (Trovan) uses a 10 digit alphanumeric beginning with two or more Os
  //    * 9 digit numeric codes were used by ??? and are sometimes displayed with an
  //      asterisk every third character
  //
  //   All NGRR chips are from Datamars via Found.org and hence begin with "98102",
  // but remember that the dog may have been already chipped when we got him. We
  // need to accept any format, and the following code isn't too fussy.  It will
  // accept any 15 digit decimal number beginning with "9" (we don't recognize
  // international codes for now), or any 10 digit hexadecimal number (covers most
  // FDXA chips).  Lastly there's a special case for the old style 9 digit chips.
  //
  //   Note that we can't really do anything to "fix" a bad chip number, except
  // for removing the asterisks or spaces from the 9 digit chips.  Everything
  // else is either good or bad as it stands.
  //--
  if (sChip.empty()) 
    {MSGS("microchip cannot be blank");   return false;}

  // FDXB or ISO chips ...
  std::tr1::regex reISO("^9\\d{14}$");
  if (std::tr1::regex_match(sChip, reISO)) return true;

  // Special hack for Pumpkin's chip!
  std::tr1::regex rePumpkin("^202\\d{12}$");
  if (std::tr1::regex_match(sChip, rePumpkin)) return true;

  // FDXA 10 digit hexadecimal chips ...
  std::tr1::regex reFDXA("^[[:xdigit:]]{10}$");
  if (std::tr1::regex_match(sChip, reFDXA)) return true;

  //   Lastly, handle the 9 digit chip numbers.  By tradition these are written
  // as three groups of three digits, separated by a space or asterisk.  Note
  // that these numbers are decimal, not hex...
  std::tr1::regex reOld("^(\\d{3})[ \\*]?(\\d{3})[ \\*]?(\\d{3})$");
  std::tr1::smatch rmChip;
  if (std::tr1::regex_search(sChip, rmChip, reOld)) {
    string sNew = rmChip.str(1) + rmChip.str(2) + rmChip.str(3);
    //MSGS("converted \"" << m_sMicrochip << "\" -> \"" << sNew << "\"");
    sChip = sNew;  return true;
  }

  // Otherwise it's bogus ...
  if (fMessage) MSGS("invalid microchip \"" << sChip << "\"");
  return false;
}


/*static*/ string CChip::GetToday()
{
  //++
  // Return today's date in the format YYYY-MM-DD.  Never fails!
  //--
  // Get time as 64-bit integer.
  time_t now;  struct tm tmnow;  char sz[256];
  time(&now);  localtime_s(&tmnow, &now);
  strftime(sz, sizeof(sz), "%Y-%m-%d", &tmnow);
  return string(sz);
}


bool CChip::Initialize (const CDogs *pDogs, const string sChip)
{
  //++
  //--
  assert(pDogs != NULL);
  m_sMicrochip = sChip;
  if (!VerifyMicrochip(m_sMicrochip)) return false;

  //   Now find the CDog record associated with this transaction.  The only way
  // we have to match them up is by the microchip number, but that's enough.
  // BTW, be careful here - the dog record still has the ORIGINAL microchip
  // number, not the one that's been fixed up by VerifyMicrochip() !
  m_pDog = pDogs->Find(sChip);
  if (m_pDog == NULL) 
    {MSGS("no dog record for microchip " << m_sMicrochip);  return false;}
  return true;
}


bool CChip::FromDog (CDog *pDog)
{
  //++
  //  Create a CChip directly from a CDog object ...
  //--
  assert(pDog != NULL);
  m_sMicrochip = pDog->GetChip();  m_pDog = pDog;
  if (!VerifyMicrochip(m_sMicrochip, false)) {
    MSGS("dog " << pDog->GetName() << " #" << pDog->GetNumber() << " has invalid microchip \"" << m_sMicrochip << "\"");
    return false;
  } else
    return true;
}


bool CChip::FromRow (const CDogs *pDogs, const CCSVRow &row)
{
  //++
  //   This method will extract data from a CSV file row and initialize this
  // chip record.
  //--

  // Extract the microchip number and find the CDog record ...
  string sChip = row[COL_NGRR_MICROCHIP_NUMBER-1];
  if (!Initialize(pDogs, sChip)) return false;

  //   Note that if a dog is adopted by an NGRR volunteer (aka a "foster
  // failure"!) then the dog record will not show the adopter data.  In this
  // case that information comes from the volunteer record, which we don't
  // have access to.  We need to copy the adopter data from the microchip
  // record to the dog record so that it's not lost ...

  // Verify that the microchip data agrees with the dog data ...
#ifdef UNUSED
  if (row[COL_NGRR_FNAME-1] != m_pDog->GetAdoptionFName())
    BADDOGS(m_pDog,"adopting party's first name doesn't match dog data - \""       << row[COL_NGRR_FNAME-1]         << "\" vs \"" << m_pDog->GetAdoptionFName() << "\"");
  if (row[COL_NGRR_LNAME-1] != m_pDog->GetAdoptionLName())
    BADDOGS(m_pDog,"adopting party's last name doesn't match dog data - \""        << row[COL_NGRR_LNAME-1]         << "\" vs \"" << m_pDog->GetAdoptionLName() << "\"");
  if (row[COL_NGRR_EMAIL_ADDRESS-1] != m_pDog->GetAdoptioneMail())
    BADDOGS(m_pDog,"adopting party's email address doesn't match dog data - \""    << row[COL_NGRR_EMAIL_ADDRESS-1] << "\" vs \"" << m_pDog->GetAdoptioneMail() << "\"");
  if (row[COL_NGRR_ADDRESS_1-1] != m_pDog->GetAdoptionAddress())
    BADDOGS(m_pDog,"adopting party's street address 1 doesn't match dog data - \"" << row[COL_NGRR_ADDRESS_1-1]     << "\" vs \"" << m_pDog->GetAdoptionAddress() << "\"");
  if (!row[COL_NGRR_ADDRESS_2-1].empty())
    BADDOGS(m_pDog,"microchip doesn't match dog data - adopting party's street address 2 not empty");
  if (row[COL_NGRR_CITY-1] != m_pDog->GetAdoptionCity())
    BADDOGS(m_pDog,"adopting party's city doesn't match dog data - \""             << row[COL_NGRR_CITY-1]          << "\" vs \""      << m_pDog->GetAdoptionCity() << "\"");
  if (row[COL_NGRR_STATE-1] != m_pDog->GetAdoptionState())
    BADDOGS(m_pDog,"adopting party's state doesn't match dog data - \""            << row[COL_NGRR_STATE-1]         << "\" vs \"" << m_pDog->GetAdoptionState() << "\"");
  if (row[COL_NGRR_ZIP_CODE-1] != m_pDog->GetAdoptionZip())
    BADDOGS(m_pDog,"adopting party's zip code doesn't match dog data - \""         << row[COL_NGRR_ZIP_CODE-1]      << "\" vs \"" << m_pDog->GetAdoptionZip() << "\"");
  if (row[COL_NGRR_HOME_PHONE-1] != m_pDog->GetAdoptionHomePhone())
    BADDOGS(m_pDog,"adopting party's phone number doesn't match dog data - \""     << row[COL_NGRR_HOME_PHONE-1]    << "\" vs \"" << m_pDog->GetAdoptionHomePhone() << "\"");
  if (row[COL_NGRR_WORK_PHONE-1] != m_pDog->GetAdoptionWorkPhone())
    BADDOGS(m_pDog,"adopting party's phone number doesn't match dog data - \""     << row[COL_NGRR_WORK_PHONE-1]    << "\" vs \"" << m_pDog->GetAdoptionWorkPhone() << "\"");
  if (row[COL_NGRR_CELL_PHONE-1] != m_pDog->GetAdoptionCellPhone())
    BADDOGS(m_pDog,"adopting party's phone number doesn't match dog data - \""     << row[COL_NGRR_CELL_PHONE-1]    << "\" vs \"" << m_pDog->GetAdoptionCellPhone() << "\"");
#endif

  if (row[COL_NGRR_PET_NAME-1] != m_pDog->GetName())
    BADDOGS(m_pDog,"dog name doesn't match dog data - \"" << row[COL_NGRR_PET_NAME-1] << "\" vs \"" << m_pDog->GetName() << "\"");
  //m_pDog->SetName(row[COL_NGRR_PET_NAME-1]);
  m_pDog->SetAdoptionFName(row[COL_NGRR_FNAME-1]);
  m_pDog->SetAdoptionLName(row[COL_NGRR_LNAME-1]);
  m_pDog->SetAdoptioneMail(row[COL_NGRR_EMAIL_ADDRESS-1]);
  m_pDog->SetAdoptionAddress(row[COL_NGRR_ADDRESS_1-1]);
  m_pDog->SetAdoptionCity(row[COL_NGRR_CITY-1]);
  m_pDog->SetAdoptionState(row[COL_NGRR_STATE-1]);
  m_pDog->SetAdoptionZip(row[COL_NGRR_ZIP_CODE-1]);
  m_pDog->SetAdoptionHomePhone(row[COL_NGRR_HOME_PHONE-1]);
  m_pDog->SetAdoptionWorkPhone(row[COL_NGRR_WORK_PHONE-1]);
  m_pDog->SetAdoptionCellPhone(row[COL_NGRR_CELL_PHONE-1]);

  // All done ...
  return true;
}


void CChip::ToRow (CCSVRow &row) const
{
  //++
  //   This method will convert this chip data into a CSV file row IN THE
  // FORMAT REQUIRED BY FOUND.ORG!  Note that this format is NOT the same as
  // the format of the Dogs Data report generated by the NGRR web page, and
  // that means that the FromRow() and ToRow() procedures are not inverse
  // operations.  That's a bit unfortunate, but we have no need to write a
  // file in the NGRR format and no need to read one in the Found.org format!
  //--
  const CDog *pDog = GetDog();
  //   If there's no adopter name, then the dog is being registered to NGRR
  // (at least temporarily, until it gets adopted).  In that case Found.org
  // will not allow use to leave all the adopter fields blank - we're required
  // to specify a first name, last name, email address and phone number.
  if (m_pDog->IsAdopted()) {
    // Use the real adopter information ...
    row[COL_FOUND_FIRST_NAME-1]         = m_pDog->GetAdoptionFName();
    row[COL_FOUND_LAST_NAME-1]          = m_pDog->GetAdoptionLName();
    row[COL_FOUND_EMAIL_ADDRESS-1]      = m_pDog->GetAdoptioneMail();
    row[COL_FOUND_ADDRESS_1-1]          = m_pDog->GetAdoptionAddress();
    // There is no second line for the street address in the NGRR database ...
    row[COL_FOUND_ADDRESS_2-1]          = "";
    row[COL_FOUND_CITY-1]               = m_pDog->GetAdoptionCity();
    row[COL_FOUND_STATE-1]              = m_pDog->GetAdoptionState();
    row[COL_FOUND_ZIP_CODE-1]           = m_pDog->GetAdoptionZip();
    row[COL_FOUND_HOME_PHONE-1]         = m_pDog->GetAdoptionHomePhone();
    row[COL_FOUND_WORK_PHONE-1]         = m_pDog->GetAdoptionWorkPhone();
    row[COL_FOUND_CELL_PHONE-1]         = m_pDog->GetAdoptionCellPhone();
  } else {
    // Not adopted - use the hardwired NGRR data ...
    row[COL_FOUND_FIRST_NAME-1]         = NGRR_FIRST_NAME;
    row[COL_FOUND_LAST_NAME-1]          = NGRR_LAST_NAME;
    row[COL_FOUND_EMAIL_ADDRESS-1]      = NGRR_EMAIL_ADDRESS;
    row[COL_FOUND_ADDRESS_1-1]          = "";
    row[COL_FOUND_ADDRESS_2-1]          = "";
    row[COL_FOUND_CITY-1]               = "";
    row[COL_FOUND_STATE-1]              = "";
    row[COL_FOUND_ZIP_CODE-1]           = "";
    row[COL_FOUND_HOME_PHONE-1]         = NGRR_PHONE_NUMBER;
    row[COL_FOUND_WORK_PHONE-1]         = "";
    row[COL_FOUND_CELL_PHONE-1]         = "";
  }
  //   Many dogs have the same name (Goldie, Bear, Max, etc) so the dog name
  // we register is actually a combination of the name and NGRR dog number.
  //   Per request from Anne, we no longer include the dog number!
//string sName = m_pDog->GetName() + " (" + std::to_string(pDog->GetNumber()) + ")";
  string sName = m_pDog->GetName();
  row[COL_FOUND_PET_NAME-1]           = sName;
  row[COL_FOUND_MICROCHIP_NUMBER-1]   = m_sMicrochip;
  //** TBA NYI TODO for the service date use today's date as MM/DD/YYYY
  // Or we could use the service date from the NGRR web page dogs data CSV ...
  string sServiceDate;  //uint32_t nDay, nMonth, nYear;
  //if (pDog->GetDispositionDate(nDay, nMonth, nYear))
  //  sServiceDate = CDog::FormatDate(nDay, nMonth, nYear);
  //else
    sServiceDate = GetToday();
  row[COL_FOUND_SERVICE_DATE-1]       = sServiceDate;
  string sDOB;
  row[COL_FOUND_DATE_OF_BIRTH-1]      = pDog->ComputeBirthday(sDOB) ? sDOB : "";
  row[COL_FOUND_SPECIES-1]            = NGRR_SPECIES;
  row[COL_FOUND_SEX-1]                = pDog->GetSex();
  //   Note that the SPAY/NEUTER field in the DIR represents the dogs' condition
  // when NGRR gets it, NOT the dogs' condition when it's adopted!  The assumption
  // is that we will ALWAYS spay or neuter our dogs before they're adopted...
  row[COL_FOUND_SPAYED_NEUTERED-1]    = "Yes";
  row[COL_FOUND_PRIMARY_BREED-1]      = NGRR_PRIMARY_BREED;
  row[COL_FOUND_SECONDARY_BREED-1]    = "";
  row[COL_FOUND_RESCUE_GROUP_EMAIL-1] = NGRR_EMAIL_ADDRESS;
  // Store the dogs' NGRR number and today's date in the notes field ...
  string sNotes = "NGRR #" + std::to_string(pDog->GetNumber());
  row[COL_FOUND_NOTES-1]              = sNotes;
}


void CChip::Display() const
{
  //++
  // Dump all this chip's data on stdout ...
  //--
  std::cout << ">>>>> Data for microchip #" << m_sMicrochip << " <<<<<" << std::endl;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CChips::DeleteAll()
{
  //++
  // Delete all the CChip objects in this collection ...  
  //--
  for (const_iterator it = begin(); it != end(); ++it)  delete it->second;
  m_mapChip.clear();
}


bool CChips::Add (CChip *pChip)
{
  //++
  // Add a chip to this collection ...
  //--
  assert(pChip != NULL);
  string sChip = pChip->GetMicrochip();
  const CDog *pDog = pChip->GetDog();

  // First, be sure that the microchip is unique ...
  if (Find(sChip) != NULL) {
    BADDOGS(pDog, "duplicate microchip \"" << sChip << "\"");
    return false;
  }

  // All's well - add this chip to the collection ....
  m_mapChip.insert({sChip, pChip});
  return true;
}


CChip *CChips::Find (string sChip) const
{
  //++
  // Find a CChip object given its microchip number ...
  //--
  const_iterator it = m_mapChip.find(sChip);
  return (it != end())  ?  it->second  :  NULL;
}


void CChips::ReadFile (const CDogs *pDogs, const string &sFileName)
{
  //++
  //   Read the entire CChips collection from the Dog Data Report CSV file
  // output by the NGRR web page ...
  //--
  CCSVFile csv;
  size_t nRows = csv.Read(sFileName, CChip::m_sNGRRHeaders);
  MSGS("Read " << nRows << " rows from " << sFileName);
  if (csv.size() == 0) return;

  for (CCSVFile::const_iterator it = csv.begin(); it != csv.end(); ++it) {
    CChip *pChip = new CChip;
    if (pChip->FromRow(pDogs, **it)) {
      if (!Add(pChip)) continue;
      //   Note that we don't verify the dog's data here - that's a fool's
      // errand as the NGRR database is full of junk.  We only verify the dog
      // data for dogs with microchips that need registering.
      pChip->GetDog()->VerifyAll();
    }
  }
  MSGS(size() << " chips loaded");
}


void CChips::WriteFile (const string &sFileName) const
{
  //++
  // Write the upload file for Found.org ...
  //--
  CCSVFile csv;
  for (const_iterator it = begin(); it != end(); ++it) {
    const CChip *pChip = it->second;
    CCSVRow row(CChip::TOTAL_FOUND_COLUMNS);
    pChip->ToRow(row);  csv.AddRow(row);
  }
  size_t nChips = csv.Write(sFileName, CChip::m_sFoundHeaders);
  MSGS("Wrote " << nChips << " rows to " << sFileName);
}
