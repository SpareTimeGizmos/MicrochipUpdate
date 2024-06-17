//++
// MicrochipUpdate.cpp - Generate update file for Found.org from NGRR database
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
//   This program generates the microchip updates for Found.org from the NGRR
// dogs database.  This is neither as easy as it sounds nor as easy as it
// should be, for lots of reasons.  First, the NGRR database doesn't really
// tell us what we want to know - i.e. which dogs need their chip registrations
// updated - and instead we have to infer this by looking for changes in the
// dog's data.  Second, the NGRR database is full of data entry errors and
// inconsistencies in usage that make our life difficult.  And lastly, we don't
// have direct access to the NGRR database and instead we have to work with
// database dumps in CSV format.  Sigh...
//
//   The basic plan is to take two dog information reports (aka DIR), one old
// and one new, and compare them.  The DIR can be exported directly from the
// NGRR web page and is essentially a dump of ALL dog data in the database in
// CSV format.  We look for dogs that have changed between the two reports - dogs
// that have been rescued; dogs that have had a microchip number added; dogs that
// have been adopted, etc, and output the appropriate updates to a new CSV file
// which is in the correct format to send to Found.org.
//
//   Along the way we report any errors or database inconsistencies that we may
// find to stderr and to a special error CSV file.  The latter is handy for
// generating a summary report of all the bad dog records that need fixing.
//
// USAGE:
//      MicrochipUpdate [-cnnnn] [-on] <old DIR> <new DIR> [[<updates>] [<errors>]]
//
//      -cnnnn    - set cutoff year to nnnn
//      -o or -o1 - old DIR is in the old format
//      -o2       - BOTH DIRs are in the old format
//      <old DIR> - the previous Dog Information Report .csv file
//      <new DIR> - the current  Dog Information Report .csv file
//      <updates> - microchip update .csv file ready to send to Found.org
//      <errors>  - error report .csv file
//
//                                          Bob Armstrong [4-Jul-2019]
//
// REVISION HISTORY:
//  4-Jul-19  RLA   New file.
// 16-OCT-19  RLA   Lots of heuristsic updates ...
//                   -> if a dog disappears from the database, only complain if it has a chip
//                   -> if the dog status is "Adoption Pending" don't complain if there's no adopter name
//                   -> complain if there's an adopter recorded, but the status is not "Adopted"
//                   -> don't complain if there's a adopter recorded but the dog's status is died/euthanized
//                   -> force an update if the adopter FName or LName has changed
//                   -> change the dog data cutoff date to 2018
//                   -> complain if a dog has a disposition date but its status is Evaluation or Available
//  4-Mar-21  RLA    Dogs that were adopted but then returned to their original owners (Nova, Gogo)
//                   still like they're adopted but have the "Returned to Owner" status set.  Need to
//                   check for that combination in CompareDogs()..
//  3-Apr-23  RLA    Update for yet another DIR file format.  Add the -o and -c
//                   command line options ('cause I'm sure this will happen again!).
//--
//000000001111111111222222222233333333334444444444555555555566666666667777777777
//234567890123456789012345678901234567890123456789012345678901234567890123456789
#include <stdio.h>              // printf(), fprintf(), ...
#include <stdlib.h>             // exit(), system(), etc ...
#include <stdint.h>	        // uint8_t, uint32_t, etc ...
#include <assert.h>             // assert() (what else??)
#include <tchar.h>              // _T(), et al ...
#include <iostream>             // std::ios, std::istream, std::cout
#include "Messages.hpp"         // ERRS(), etc ...
#include "CSVRow.hpp"           // one row of a spreadsheet
#include "CSVFile.hpp"          // a collection of spreadsheet rows
#include "Dog.hpp"              // CDog data and CDogs collection
#include "Chip.hpp"             // CChip data and CChips collection

// Useful definitions ...
#define STREQL(a,b)     (strcmp(a,b) == 0)
#define STRNEQL(a,b,l)  (strncmp(a,b,l) == 0)

// Globals ...
#define DEFAULT_EXTENSION ".csv"      // default file type for all csv files
string g_sOldDogsFile("");            // old DIR report csv file
string g_sNewDogsFile("");            // new  "     "    "   "
bool   g_fOldDogsFormat(true);        // true if the old dogs file is the new format
bool   g_fNewDogsFormat(true);        //   "  "   "  new   "   "   "   "   "     "
int    g_nCutoffYear(2019);           // dogs before 1-JAN-year are ignored
string g_sUpdatesFile("updates.csv"); // output file for Found.org
string g_sErrorsFile("errors.csv");   // error listing file


void CompareDogs (const CDogs &OldDogs, CDogs &NewDogs)
{
  //++
  //   Compare a new dog database, after the csv file has been read into a 
  // CDogs collection, with an older dog database.  This looks for any errors
  // or inconsistencies and reports them.  It also tries to figure out which
  // dogs in the new database need to be uploaded to Found.org and marks
  // them with SetUpdateRequired().  It doesn't actually generate an output
  // file for Found.org though - that's another job...
  //--
  MSGS("Comparing " << OldDogs.DogCount() << " old dogs with " << NewDogs.DogCount() << " new dogs ...");

  //   Part 0 - Sanitize all the new dog data ...  We should probably verify
  // all the old dog data too, but that never ends up in the update file ...
  //
  //  NOTE - we used to do this, but there's just too much spurious garbage
  // in the database.  Now we only validate those dog records that we actually
  // need to register with Found.org, and that's done in the BuildUpdates()
  // method.  All the other problems we just ignore ...
//for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
//  CDog *pNewDog = it->second;
//  pNewDog->VerifyAll();
//}

  //   Part 1 - we never delete a dog record, so all the dogs in the OldDogs
  // collection should exist in the NewDogs.  Warn about any that don't follow
  // this rule.
  for (CDogs::dog_number_const_iterator it = OldDogs.dog_begin(); it != OldDogs.dog_end(); ++it) {
    const CDog *pOldDog = it->second;
    if (NewDogs.Find(pOldDog->GetNumber()) == NULL) {
      //   Turns out that dogs disappear from the database more often than you
      // might think.  Don't ask me to explain why, but only report a problem
      // if the dog had a microchip registered.  Otherwise I guess we don't care.
      if (pOldDog->HasChip())
        BADDOGS(pOldDog, "has microchip " + pOldDog->GetChip() + " but is not found in new dog report");
    }
  }

  //   Conversely, any dog which is in the new dogs but not in the old dogs
  // must have been recently acquired.  In that case the new dog MUST have a
  // microchip number recorded too, and we'll need to update Found.org with
  // that information.
  //
  //   There's a slight hack here too - sometimes an A/C may change the chip
  // number recorded for a dog, or an A/C might forget to enter the chip number
  // now but then go back and re-enter it later.  We need to detect both of
  // those cases too and update Found.org as well.
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    if (pNewDog->IsDead() || pNewDog->IsReturned()) continue;
    const CDog *pOldDog = OldDogs.Find(pNewDog->GetNumber());
    if (pOldDog == NULL) {
      // Dog was recently acquired.  As long as it has a microchip, register it!
      MSGS("dog " << pNewDog->GetName() << " #" << pNewDog->GetNumber() << " was acquired");
      if (pNewDog->GetChip().empty()) {
        BADDOGS(pNewDog, "no microchip number recorded");
      } else {
        pNewDog->SetUpdateRequired();
      }
    } else if (pOldDog->GetChip().empty() && !pNewDog->GetChip().empty()) {
      // The dog didn't have a chip number before but does now - register it!
      MSGS("dog " << pNewDog->GetName() << " #" << pNewDog->GetNumber() << " microchip was added");
      pNewDog->SetUpdateRequired();
    } else if (pOldDog->GetChip() != pNewDog->GetChip()) {
      //  The dog's microchip number was changed.  Note that we can't fix this
      // by updating Found.org, so you're on your own!
      BADDOGS(pNewDog, "microchip number changed - was \"" << pOldDog->GetChip() << "\" is \"" << pNewDog->GetChip() << "\"");
    }
  }

  //   If the dog's status says it's adopted but there's no adopter name or
  // address recorded, then issue a warning.  It was likely a dog that was
  // adopted by an NGRR member - there's a bug in the web page that prevents
  // the adopter information from being recorded on these dogs...
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    if ((pNewDog->GetStatus() == "Adopted") /*|| (pNewDog->GetStatus() == "Adoption Pending")*/) {
      if (pNewDog->GetAdoptionFName().empty() && pNewDog->GetAdoptionLName().empty())
        //BADDOGS(pNewDog, "status is ADOPTED but no adopter name is recorded");
        BADDOGS(pNewDog, pNewDog->GetStatus() + " but no adopting party is recorded");
    }
  }

  //   Conversely, if there's an adopter name recorded and the dog's status is
  // NOT adopted, then complain about that too...
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    if (!pNewDog->GetAdoptionFName().empty() || !pNewDog->GetAdoptionLName().empty()) {
      if ((pNewDog->GetStatus() != "Adopted") && (pNewDog->GetStatus() != "Adoption Pending")) {
        //   There are dogs in the database that are recorded as died or euthanized
        // but are still shown as adopted.  Not sure how that happened (why would
        // we record a dog's death AFTER it was adopted??) but we'll ignore those.
        if (pNewDog->IsDead() || pNewDog->IsReturned()) continue;
        BADDOGS(pNewDog, " adopting party is recorded but status is " + pNewDog->GetStatus());
      }
    }
  }

  //   If the dog has a disposition date (which normally means that the dog was
  // adopted, returned, died, or otherwise "disposed of") BUT the status is still
  // Evaluation or Available, then complain...
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    if (!pNewDog->GetDispositionDate().empty()) {
      //   Don't know why, but a lot of dogs have a disposition date that looks
      // like "0000-00-00" ...  Just ignore those.
      if (pNewDog->GetDispositionDate() == "0000-00-00") continue;
      if ((pNewDog->GetStatus() == "Evaluation") || (pNewDog->GetStatus() == "Available"))
        BADDOGS(pNewDog, "disposition date is " + pNewDog->GetDispositionDate() + " but status is " + pNewDog->GetStatus());
    }
  }

  //   Now go thru the new dogs and look for ones that are adopted now but
  // weren't adopted last time around.  These dogs were recently adopted, and
  // also need registering with Found.org.  And just to be safe, if the dog
  // both was and is adopted, see if the adopting family has changed.
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    const CDog *pOldDog = OldDogs.Find(pNewDog->GetNumber());
    if (pNewDog->IsDead() || pNewDog->IsReturned()) continue;
    if (!pNewDog->IsAdopted()) continue;
    if ((pOldDog != NULL) && pOldDog->IsAdopted()) {
      // Was adopted before and is adopted now ...
      if ((pOldDog->GetAdoptionFName() != pNewDog->GetAdoptionFName())
          || (pOldDog->GetAdoptionLName() != pNewDog->GetAdoptionLName()))
          BADDOGS(pOldDog, "adopting family changed");
          // Should an update be required here??!!  Probably...
    } else {
      // Was recently adopted ...
      MSGS("dog " << pNewDog->GetName() << " #" << pNewDog->GetNumber() << " was adopted by " << pNewDog->GetAdoptionFName() << " " << pNewDog->GetAdoptionLName());
      pNewDog->SetUpdateRequired();
    }
  }

  //   Lastly, look for dogs that were returned to NGRR.  These dogs would have
  // been adopted last time around but are not adopted now.
  for (CDogs::dog_number_const_iterator it = NewDogs.dog_begin(); it != NewDogs.dog_end(); ++it) {
    CDog *pNewDog = it->second;
    const CDog *pOldDog = OldDogs.Find(pNewDog->GetNumber());
    if ((pOldDog == NULL) || !pOldDog->IsAdopted()) continue;
    if (!pNewDog->IsAdopted()) {
      // Was adopted before and is NOT adopted now ...
      MSGS("dog " << pNewDog->GetName() << " #" << pNewDog->GetNumber() << " was returned to NGRR");
      pNewDog->SetUpdateRequired();
    }
  }
}


void BuildUpdates (CDogs &Dogs, CChips &Chips)
{
  //++
  //   This method goes thru the new dogs database and, for each dog that has
  // the SetUpdateRequired() flag set, adds it to the CChips collection.  The
  // latter is used to generate the update file for Found.org.
  //--
  Chips.DeleteAll();
  for (CDogs::dog_number_iterator it = Dogs.dog_begin(); it != Dogs.dog_end(); ++it) {
    CDog *pDog = it->second;
    if (!pDog->IsUpdateRequired()) continue;
    if (pDog->GetChip().empty()) {
      BADDOGS(pDog, "requires update but has no microchip!");
    } else {
      CChip *pChip = new CChip;
      pDog->VerifyAll();
      if (pChip->FromDog(pDog)) Chips.Add(pChip);
    }
  }
}


void PrintUsage(void)
{
  //++
  //   Print the help (usage instructions) for this program and then exit.
  // It's called when we're run with no arguments, or if the argument list is
  // bogus...
  //--
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "\tMicrochipUpdate [-cnnnn] [-on] <old DIR> <new DIR> [[<updates>] [<errors>]]\n\n");
  fprintf(stderr, "\t-cnnnn    - set cutoff year to nnnn\n");
  fprintf(stderr, "\t-o or -o1 - old DIR is in the old format\n");
  fprintf(stderr, "\t-o2       - BOTH DIRs are in the old format\n");
  fprintf(stderr, "\t<old DIR> - the previous Dog Information Report .csv file\n");
  fprintf(stderr, "\t<new DIR> - the current  Dog Information Report .csv file\n");
  fprintf(stderr, "\t<updates> - microchip update .csv file ready to send to Found.org\n");
  fprintf(stderr, "\t<errors>  - error report .csv file\n");
  fprintf(stderr, "\n");
}


string ApplyDefaultExtension (const string sFileName, const char *pszType)
{
  //++
  //   This method will apply a default extension (e.g. ".csv") to the file name
  // specified and return the result.  It's not super smart, but it's enough...
  //--

  // First use the WIN32 _splitpath() function to parse the original file name ...
  char szDrive[_MAX_DRIVE+1], szDirectory[_MAX_DIR+1];
  char szFileName[_MAX_FNAME+1], szExtension[_MAX_EXT+1];
  errno_t err = _splitpath_s(sFileName.c_str(), szDrive, sizeof(szDrive),
                             szDirectory, sizeof(szDirectory), szFileName, sizeof(szFileName),
                             szExtension, sizeof(szExtension));
  if (err != 0) return sFileName;

  // if the extension is null, then apply the default ...
  if (strlen(szExtension) == 0) strcpy_s(szExtension, sizeof(szExtension), pszType);

  // Put the file name back together and we're done...
  string sResult = string(szDrive) + string(szDirectory) + string(szFileName) + string(szExtension);
  return sResult;
}


bool ParseArguments (int argc, char *argv[])
{
  //++
  // Parse the command line arguments and extract the file names ...
  //--
  int nArg = 1;  --argc;

  //   Check for -o and -c options first ...  If present, these have to be
  // at the beginning of the command line.  Actually there's no reason why
  // they "have" to be, but this parser is pretty simple minded...
  while ((argc > 0) && (argv[nArg][0] == '-')) {
    if (STREQL(argv[nArg],"-o") || STREQL(argv[nArg], "-o1")) {
      g_fOldDogsFormat = false;
    } else if (STREQL(argv[nArg], "-o2")) {
      g_fOldDogsFormat = g_fNewDogsFormat = false;
    } else if (STRNEQL(argv[nArg], "-c", 2)) {
      char *psz;
      if (strlen(argv[nArg]) < 3) return false;
      g_nCutoffYear = (int) strtoul(&argv[nArg][2], &psz, 10);
      if (*psz != '\0') return false;
      if ((g_nCutoffYear < 2010) || (g_nCutoffYear > 2050)) return false;
    } else
      return false;
    ++nArg;  --argc;
  }

  // The OLD DIR and NEW DIR file names are required ...
  if (argc < 2) return false;
  g_sOldDogsFile = ApplyDefaultExtension(argv[nArg++], DEFAULT_EXTENSION);
  g_sNewDogsFile = ApplyDefaultExtension(argv[nArg++], DEFAULT_EXTENSION);
  argc -= 2;

  // Now parse the optional file names ...
  if (argc > 0) {
    g_sUpdatesFile = ApplyDefaultExtension(argv[nArg++], DEFAULT_EXTENSION); --argc;
  }
  if (argc > 0) {
    g_sErrorsFile = ApplyDefaultExtension(argv[nArg++], DEFAULT_EXTENSION);  --argc;
  }
  return true;
}


int main (int argc, char *argv[])
{
  //++
  //   Parse the command line, read the input files, compare the two reports,
  // and generate an update file for Found.org.  Easy!
  //--
  if (ParseArguments(argc, argv)) {
    CBadDogs baddogs(g_sErrorsFile);  CDogs OldDogs, NewDogs;
    OldDogs.ReadFile(g_sOldDogsFile, g_nCutoffYear, g_fOldDogsFormat);
    NewDogs.ReadFile(g_sNewDogsFile, g_nCutoffYear, g_fNewDogsFormat);
    CompareDogs(OldDogs, NewDogs);
    CChips Chips;
    BuildUpdates(NewDogs, Chips);
    Chips.WriteFile(g_sUpdatesFile);
  } else
    PrintUsage();
  exit(1);
}
