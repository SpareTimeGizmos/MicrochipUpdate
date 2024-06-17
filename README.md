# MicrochipUpdate

  This program generates the microchip updates for Found.org from the NGRR
(NorCal Golden Retriever Rescue, www.ngrr.org) dogs database.  This is neither
as easy as it sounds nor as easy as it should be, for lots of reasons.  First,
the NGRR database doesn't really tell us what we want to know - i.e. which dogs
need their chip registrations updated - and instead we have to infer this by
looking for changes in the dog's data.  Second, the NGRR database is full of
data entry errors and inconsistencies in usage that make our life difficult.
And lastly, we don't have direct access to the NGRR database and instead we
have to work with database dumps in CSV format.  Sigh...

   The basic plan is to take two dog information reports (aka DIR), one old
and one new, and compare them.  The DIR can be exported directly from the
NGRR web page and is essentially a dump of ALL dog data in the database in
CSV format.  We look for dogs that have changed between the two reports - dogs
that have been rescued; dogs that have had a microchip number added; dogs that
have been adopted, etc, and output the appropriate updates to a new CSV file
which is in the correct format to send to Found.org.

  Along the way we report any errors or database inconsistencies that we may
find to stderr and to a special error CSV file.  The latter is handy for
generating a summary report of all the bad dog records that need fixing.
