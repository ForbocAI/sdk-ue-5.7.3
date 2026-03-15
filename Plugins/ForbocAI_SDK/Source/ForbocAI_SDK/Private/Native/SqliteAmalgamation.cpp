/**
 * Compiles the sqlite3 + sqlite-vec amalgamation as part of the UE module.
 *
 * UBT auto-discovers .cpp files under Source/ but NOT .c files in ThirdParty/.
 * This wrapper #includes the amalgamation sources so they are compiled into the
 * plugin when WITH_FORBOC_SQLITE_VEC is enabled.
 *
 * User Story: As native memory persistence, I need sqlite3 compiled into the
 * plugin so vector search and memory storage link correctly.
 */

#include "CoreMinimal.h"

#if WITH_FORBOC_SQLITE_VEC

// Suppress third-party warnings
THIRD_PARTY_INCLUDES_START

// sqlite3 amalgamation — ~240k LOC, compiles as C
extern "C" {
#include "sqlite3.c"
}

// sqlite-vec extension — vector similarity search
extern "C" {
#include "vec0.c"
}

THIRD_PARTY_INCLUDES_END

#endif // WITH_FORBOC_SQLITE_VEC
