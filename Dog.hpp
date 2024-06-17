//++
// Dog.hpp - CDog dog data class and CDogs collection class
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
//   This file defines two classes - a CDog class, which encapsulates all dog
// related data from the NGRR database, and a CDogs class, which is a collection
// of CDog objects.  
//
//                                              Bob Armstrong [8-Jul-2019]
//
// REVISION HISTORY:
//  8-JUL-19  RLA   New file.
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#pragma once
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <string>               // C++ std::string class, et al ...
#include <unordered_map>        // C++ std::unordered_map (aka a hash table)
#include <regex>                // regular expression matching ...
using std::size_t;              // ...
using std::string;              // ...
using std::unordered_map;       // ...
class CCSVRow;                  // ...


class CDog {
  //++
  // A single dogs' data ...
  //--

public:
  enum {
    // Magic number constants ...
    MAXDOG                              = 99999,// largest possible NGRR dog number
    //   Having constants for the columns in the Dog Information Report used to be a good
    // idea, but now we've gone thru no less than three different iterations of the DIR
    // format.  It's just too complicated to have "old old", "old new" and "new new" 
    // constants for everything, so now it's up to the FromRow() and ToRow() methods to
    // work it out...
//    // Column numbers for the dog information report (DIR) generated by the NGRR web page ...
//    COL_DOG_NAME			=  1,	// dog name
//    COL_DOG_NUMBER			=  2,	// NGRR dog number (guaranteed to be unique!)
//    COL_MICROCHIP_NUMBER		=  3,	// microchip number
//    COL_DOG_AGE			=  4,	// dog's age (when this record was created!)
//    COL_DOG_SEX			=  5,	// Male/Female
//    COL_DOG_NEUTER			=  6,	// Yes/No
//    COL_DOG_STATUS			=  7,	// Adopted/Pending/Died/Evaluation/etc
//    COL_DOG_LOCATION			=  8,	// city name
//    COL_HOW_ACQUIRED			=  9,	// Surrender/Shelter/Craigslist/etc
//    COL_DATE_ACQUIRED			= 10,	// date
//    COL_PRIMARY_CONTACT_FNAME		= 11,	// usually this is the A/C, but not always
//    COL_PRIMARY_CONTACT_LNAME		= 12,	// ...
//    COL_SURRENDER_FNAME		= 13,	// Surrendering party's first name
//    COL_SURRENDER_LNAME		= 14,	// ... last name
//    COL_SURRENDER_ADDRESS		= 15,	// ... street address
//    COL_SURRENDER_CITY		= 16,	// ... city
//    COL_SURRENDER_STATE		= 17,	// ... state
//    COL_SURRENDER_ZIP_CODE		= 18,	// ... zip code
//    COL_ORIGINATING_AREA		= 19,	// NGRR area associated
//    COL_COUNTY                        = ??,   // new with the January 2023 format
//    //   For reasons known only to Will, there are two slightly different versions
//    // of the dog information report out there.  The only difference between the
//    // "old" and "new" version is that the Adoption FName/LName and AC FName/
//    // LName columns have been swapped around.  
//    COL_OLD_ADOPTION_FNAME		= 20,	// Adopting party's first name
//    COL_OLD_ADOPTION_LNAME		= 21,	// ... last name
//    COL_OLD_AC_FNAME			= 22,	// area's A/C name
//    COL_OLD_AC_LNAME			= 23,	// ...
//    COL_NEW_AC_FNAME			= 20,	// new style
//    COL_NEW_AC_LNAME			= 21,	// ...
//    COL_NEW_ADOPTION_FNAME		= 22,	// ...
//    COL_NEW_ADOPTION_LNAME		= 23,	// ... 
//    // And once again things are the same ...
//    COL_ADOPTION_ADDRESS		= 24,	// Adopting party's street address
//    COL_ADOPTION_CITY			= 25,	// ... city
//    COL_ADOPTION_STATE		= 26,	// ... state
//    COL_ADOPTION_ZIP_CODE		= 27,	// ... zip code
//    COL_ADOPTION_AREA			= 28,	// ... NGRR area (if the dog was moved)
//    COL_ADOPTION_EMAIL		= 29,	// ... email address
//    COL_ADOPTION_HOME_PHONE		= 30,	// ... phone number
//    COL_ADOPTION_WORK_PHONE		= 31,	// ... phone number
//    COL_ADOPTION_CELL_PHONE		= 32,	// ... phone number
//    COL_ADOPTION_STATUS		= 33,	// ...
//    COL_ADOPTION_OR_DISPOSITION_DATE	= 34,	// date adoption contract was recorded
      TOTAL_OLD_COLUMNS			= 35,	// number of colums in the old dog data CSV
      TOTAL_NEW_COLUMNS			= 36	//   "    "    "     "  "  new dog  "    "
  };
  // This is the expected header row for the dog information report ...
  static const string m_sOldColumnHeaders;      // Old style headers
  static const string m_sNewColumnHeaders;      // New style headers

public:
  // Constructors ...
  CDog() {}
  // Destructor ...
  virtual ~CDog() {};

  // CDog public properties ...
public:
  //   Get this dog's NGRR and/or microchip number.  Note that there are no
  // corresponding set functions for these - since these fields are used as
  // keys for the CDogs collection, we don't allow them to be changed.
  uint32_t GetNumber() const {return m_nNumber;}
  string GetChip() const {return m_sMicrochip;}
  bool HasChip() const {return !m_sMicrochip.empty();}
  // Figure out which NGRR person is responsible for this dog ...
  string GetResponsiblePerson() const;
  // Return the other parts of the dog record ...
  const string GetName() const {return m_sName;}
//const string GetAge() const {return m_sAge;}
  const string GetSex() const {return m_sSex;}
  const string GetNeuter() const {return m_sNeuter;}
  const string GetStatus() const {return m_sStatus;}
//const string GetLocation() const {return m_sLocation;}
//const string GetHowAcquired() const {return m_sHowAcquired;}
  // Parse and return the date acquired and the date adopted ...
  static bool ParseDate (const string &sDate, uint32_t &nDay, uint32_t &nMonth, uint32_t &nYear);
  const string GetDateAcquired() const {return m_sDateAcquired;}
  bool GetDateAcquired (uint32_t &nDay, uint32_t &nMonth, uint32_t &nYear) const
    {return ParseDate(GetDateAcquired(), nDay, nMonth, nYear);}
  const string GetDispositionDate() const {return m_sDispositionDate;}
  bool GetDispositionDate (uint32_t &nDay, uint32_t &nMonth, uint32_t &nYear) const
    {return ParseDate(GetDispositionDate(), nDay, nMonth, nYear);}
//const string GetPrimaryContactFName() const {return m_sPrimaryContactFName;}
//const string GetPrimaryContactLName() const {return m_sPrimaryContactLName;}
//const string GetSurrenderFName() const {return m_sSurrenderFName;}
//const string GetSurrenderLName() const {return m_sSurrenderLName;}
//const string GetSurrenderAddress() const {return m_sSurrenderAddress;}
//const string GetSurrenderCity() const {return m_sSurrenderCity;}
//const string GetSurrenderState() const {return m_sSurrenderState;}
//const string GetSurrenderZipCode() const {return m_sSurrenderZipCode;}
//const string GetOriginatingArea() const {return m_sOriginatingArea;}
  const string GetAdoptionFName() const {return m_sAdoptionFName;}
  const string GetAdoptionLName() const {return m_sAdoptionLName;}
  const string GetACFName() const {return m_sACFName;}
  const string GetACLName() const {return m_sACLName;}
  const string GetAdoptionAddress() const {return m_sAdoptionAddress;}
  const string GetAdoptionCity() const {return m_sAdoptionCity;}
  const string GetAdoptionState() const {return m_sAdoptionState;}
  const string GetAdoptionZip() const {return m_sAdoptionZip;}
  const string GetAdoptionArea() const {return m_sAdoptionArea;}
  const string GetAdoptioneMail() const {return m_sAdoptioneMail;}
  const string GetAdoptionHomePhone() const {return m_sAdoptionHomePhone;}
  const string GetAdoptionWorkPhone() const {return m_sAdoptionWorkPhone;}
  const string GetAdoptionCellPhone() const {return m_sAdoptionCellPhone;}
  const string GetAdoptionStatus() const {return m_sAdoptionStatus;}
  //   These are the dog data fields that we can change.  There's no
  // why we couldn't set more of them, but there's no need ...
  void SetAdoptionFName (const string &sName) {m_sAdoptionFName = sName;}
  void SetAdoptionLName(const string &sName) {m_sAdoptionLName = sName;}
  void SetACFName (const string &sName) {m_sACFName = sName;}
  void SetACLName (const string &sName) {m_sACLName = sName;}
  void SetAdoptionAddress (const string &sAddr) {m_sAdoptionAddress = sAddr;}
  void SetAdoptionCity (const string &sCity) {m_sAdoptionCity = sCity;}
  void SetAdoptionState (const string &sState) {m_sAdoptionState = sState;}
  void SetAdoptionZip (const string &sZip) {m_sAdoptionZip = sZip;}
  void SetAdoptioneMail (const string &seMail) {m_sAdoptioneMail = seMail;}
  void SetAdoptionHomePhone (const string &sPhone) {m_sAdoptionHomePhone = sPhone;}
  void SetAdoptionWorkPhone(const string &sPhone) {m_sAdoptionWorkPhone = sPhone;}
  void SetAdoptionCellPhone (const string &sPhone) {m_sAdoptionCellPhone = sPhone;}
  //   Test the dog status for various conditions.  Beware of depending
  // on these results, because the database is none too accurate!
  bool IsEuthanized() const {return m_sStatus.find("Euthanized") != string::npos;}
  bool IsDied() const {return m_sStatus.find("Died") != string::npos;}
  bool IsDead() const {return IsEuthanized() || IsDied();}
  bool IsReturned() const {return m_sStatus.find("Returned") != string::npos;}
  //   A dog is adopted if the adopter first or last name is not null.  You might
  // think the right thing to do would be to check the status for "Adopted" or
  // maybe "Adoption Pending", but that doesn't work in our database...
  //bool IsAdopted() const {return (m_sStatus == "Adopted") || (m_sStatus == "Adoption Pending");}
  bool IsAdopted() const {return !m_sAdoptionFName.empty() || !m_sAdoptionLName.empty();}
  // Check the dog's acquisition date and verify that it is after nYear ...
  bool WasAcquiredAfter (uint32_t nYear) const;
  // Test, set or clear the update required flag ...
  bool IsUpdateRequired() const {return m_fUpdateRequired;}
  void SetUpdateRequired (bool fUpdate=true) {m_fUpdateRequired = fUpdate;}

  // CDog public methods ...
public:
  // Initialize this CDog object ...
  void Initialize (uint32_t nDog=0);
  // Extract data from a CSV file row ...
  bool FromRow (const CCSVRow &row, bool fNew=false);
  // Convert data to a CSV file row ...
  void ToRow (CCSVRow &row, bool fNew=false) const;
  // Display this dog on stdout ...
  void Display() const;
  // Verify (and fix if necessary) various phone numbers ...
  bool VerifyHomePhone(bool fQuiet=false) {return VerifyPhone("home", m_sAdoptionHomePhone, fQuiet);}
  bool VerifyCellPhone(bool fQuiet=false) {return VerifyPhone("cell", m_sAdoptionCellPhone, fQuiet);}
  bool VerifyWorkPhone(bool fQuiet=false) {return VerifyPhone("work", m_sAdoptionWorkPhone, fQuiet);}
  // Verify (can't fix!) the adopter and surrender zip codes ...
  bool VerifyAdoptionZip() {return VerifyZip(m_sAdoptionZip);}
  bool VerifySurrenderZip() {return VerifyZip(m_sSurrenderZipCode);}
  // Verify (can't fix!) the adopter's email address ...
  bool VerifyAdoptioneMail() {return VerifyeMail(m_sAdoptioneMail);}
  // Verify (can't fix!) the adopter or surrender state names ...
  bool VerifyAdoptionState() {return VerifyState(m_sAdoptionState);}
  bool VerifySurrenderState() {return VerifyState(m_sSurrenderState);}
  // Verify the sex (Male/Female) of a dog ...
  bool VerifySex();
  // Verify the spay/neuter status (Yes/No) of a dog ...
  bool VerifySpayNeuter();
  // Verify (and maybe fix) ALL dog data !
  bool VerifyAll();
  // Attempt to compute the dog's date of birth ...
  static string FormatDate (uint32_t nDay, uint32_t nMonth, uint32_t nYear);
  bool ComputeBirthday (string &sDOB) const;

  // Private internal CDog methods ...
protected:
  // Convert a string to lower case (why doesn't C++ have this already?!??)...
  static string tolower (const string &str);
  // Simplified regex match ...
  bool Match (const string &str, std::tr1::smatch &match, const string &sre) const
    {std::tr1::regex re(sre);  return std::tr1::regex_search(str, match, re);}
  // Verify a phone number (work, ccell or home) ...
  bool VerifyPhone (const string &sWhich, string &sPhone, bool fQuiet=false) const;
  // Verify a zip code ...
  bool VerifyZip (string &sZip) const;
  // Verify an email address ...
  bool VerifyeMail (string &seMail) const;
  // Verify a USPS two letter state abbreviation ...
  bool VerifyState (string &sState) const;


  // Local CDog members ...
protected:
  //   Note that the NGRR dog number is the only thing stored as an actual
  // numeric value - this is assumed to always be valid.  Everything else is
  // stored as a string and, depending on the quality of the data, may be valid
  // or may be total garbage...
  uint32_t  m_nNumber;			// COL_DOG_NUMBER
  string    m_sName;			// COL_DOG_NAME
  string    m_sMicrochip;		// COL_MICROCHIP_NUMBER
  string    m_sAge;			// COL_DOG_AGE
  string    m_sSex;			// COL_DOG_SEX
  string    m_sNeuter;			// COL_DOG_NEUTER
  string    m_sStatus;			// COL_DOG_STATUS
  string    m_sLocation;		// COL_DOG_LOCATION
  string    m_sHowAcquired;		// COL_HOW_ACQUIRED
  string    m_sDateAcquired;		// COL_DATE_ACQUIRED
  string    m_sPrimaryContactFName;	// COL_PRIMARY_CONTACT_FNAME
  string    m_sPrimaryContactLName;	// COL_PRIMARY_CONTACT_LNAME
  string    m_sSurrenderFName;		// COL_SURRENDER_FNAME
  string    m_sSurrenderLName;		// COL_SURRENDER_LNAME
  string    m_sSurrenderAddress;	// COL_SURRENDER_ADDRESS
  string    m_sSurrenderCity;		// COL_SURRENDER_CITY
  string    m_sSurrenderState;		// COL_SURRENDER_STATE
  string    m_sSurrenderZipCode;	// COL_SURRENDER_ZIP_CODE
  string    m_sOriginatingArea;		// COL_ORIGINATING_AREA
  string    m_sAdoptionFName;		// COL_ADOPTION_FNAME
  string    m_sAdoptionLName;		// COL_ADOPTION_LNAME
  string    m_sACFName;			// COL_AC_FNAME
  string    m_sACLName;			// COL_AC_LNAME
  string    m_sAdoptionAddress;		// COL_ADOPTION_ADDRESS
  string    m_sAdoptionCity;		// COL_ADOPTION_CITY
  string    m_sAdoptionState;		// COL_ADOPTION_STATE
  string    m_sAdoptionZip;		// COL_ADOPTION_ZIP_CODE
  string    m_sAdoptionArea;		// COL_ADOPTION_AREA
  string    m_sAdoptioneMail;		// COL_ADOPTION_EMAIL
  string    m_sAdoptionHomePhone;	// COL_ADOPTION_HOME_PHONE
  string    m_sAdoptionWorkPhone;	// COL_ADOPTION_WORK_PHONE
  string    m_sAdoptionCellPhone;	// COL_ADOPTION_CELL_PHONE
  string    m_sAdoptionStatus;		// COL_ADOPTION_STATUS
  string    m_sDispositionDate;		// COL_ADOPTION_OR_DISPOSITION_DATE
  bool      m_fUpdateRequired;          // TRUE if Found.org needs to be updated
};


class CDogs {
  //++
  //   Collection of CDog objects ...  There are a couple of thins worth knowing
  // about this collection.  The first is that we actually keep TWO unordered
  // maps of the CDog objects - one is hased by the NGRR dog number, and the
  // second is hashed by the microchip number.  All dogs have a unique NGRR
  // number and are entered in the first map, but not all dogs have a microchip
  // number recorded.  Only those dogs with a non-blank chip number are contained
  // in the second map.
  //
  //   The second important thing to know is that this object will delete all
  // the CDog objects when it is deleted.  The caller creates the CDog object
  // and then adds it to this collection, and from there on the collection owns
  // the object.
  //--

public:
  // Define the dog collection hashes ...
  typedef unordered_map<uint32_t, CDog *> DOG_NUMBER_HASH;
  typedef DOG_NUMBER_HASH::iterator dog_number_iterator;
  typedef DOG_NUMBER_HASH::const_iterator dog_number_const_iterator;
  typedef unordered_map<string, CDog *> MICROCHIP_HASH;
  typedef MICROCHIP_HASH::iterator microchip_iterator;
  typedef MICROCHIP_HASH::const_iterator microchip_const_iterator;

public:
  // Constructors ...
  CDogs()  {m_mapChip.clear();  m_mapNumber.clear();}
  // Copy and assignment constructors ...
  CDogs (const CDogs &dogs) = delete;
  CDogs& operator= (const CDogs &dogs) = delete;
  // Destructor ...
  virtual ~CDogs()  {DeleteAll();}

  // CDogs collection properties ...
public:
  // Delegate the iterators for NGRR dog numbers and microchips ...
  dog_number_iterator dog_begin() {return m_mapNumber.begin();}
  const dog_number_const_iterator dog_begin() const {return m_mapNumber.begin();}
  dog_number_iterator dog_end() {return m_mapNumber.end();}
  const dog_number_const_iterator dog_end() const {return m_mapNumber.end();}
  microchip_iterator chip_begin() {return m_mapChip.begin();}
  const microchip_const_iterator chip_begin() const {return m_mapChip.begin();}
  microchip_iterator chip_end() {return m_mapChip.end();}
  const microchip_const_iterator chip_end() const {return m_mapChip.end();}
  // Return the number of dogs or dogs with microchips ...
  size_t DogCount() const {return m_mapNumber.size();}
  size_t ChipCount() const {return m_mapChip.size();}
  // Delegate array form addressing for dog numbers and microchips ...
  const CDog* operator[] (uint32_t nDog) const
    {const CDog *p = Find(nDog);  assert(p != NULL);  return p;}
  CDog* operator[] (uint32_t nDog) 
    {CDog *p = Find(nDog);  assert(p != NULL);  return p;}
  const CDog* operator[] (string sChip) const
    {const CDog *p = Find(sChip);  assert(p != NULL);  return p;}
  CDog* operator[] (string sChip) 
    {CDog *p = Find(sChip);  assert(p != NULL);  return p;}

  // CDogs public methods ...
public:
  // Delete all the CDog objects in this collection ....
  void DeleteAll();
  // Add a dog to this collection ...
  bool Add (CDog *pDog);
  bool Add (const CCSVRow &row);
  // Find dogs by number (integer) or microchip (string) ...
  CDog *Find (uint32_t nDog) const;
  CDog *Find (string sChip) const;
  // Read or write this collection from/to a CSV file ...
  void ReadFile (const string &sFileName, uint32_t nYear=0, bool fNew=false);
  void WriteFile (const string &sFileName, bool fNew=false) const;
  // Verify that all new dogs have a microchip ...
  void VerifyNewMicrochips(uint32_t nYear=2019) const;

  // Private internal CDogs methods ...
protected:

  // Local CDogs members ...
protected:
  DOG_NUMBER_HASH m_mapNumber;  // NGRR dog # hash table
  MICROCHIP_HASH  m_mapChip;    // microchip number hash table
};
