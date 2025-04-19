#ifndef FAIRNESS_H
#define FAIRNESS_H

#include "postgres.h"

#include "access/sysattr.h"
#include "access/table.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "catalog/partition.h"
#include "commands/matview.h"
#include "commands/trigger.h"
#include "executor/executor.h"
#include "executor/execPartition.h"
#include "executor/nodeSubplan.h"
#include "foreign/fdwapi.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "nodes/queryjumble.h"
#include "parser/parse_relation.h"
#include "pgstat.h"
#include "rewrite/rewriteHandler.h"
#include "storage/lmgr.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/backend_status.h"
#include "utils/lsyscache.h"
#include "utils/partcache.h"
#include "utils/plancache.h"
#include "utils/rls.h"
#include "utils/snapmgr.h"
#include "utils/elog.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "lib/my_vector.h"
#include "access/printtup.h"
#include "lib/rbtree.h"  // For RBTree structure and related functions
#include "lib/pairingheap.h"
#include "catalog/namespace.h"  // for get_rel_name()
#include "access/heapam.h"
#include "access/htup_details.h"
#include "utils/rel.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"
#include "utils/typcache.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "catalog/pg_type.h"
#include "executor/executor.h"
#include "parser/parsetree.h"

/* Define key/value types */
typedef struct MyEntry {
    const char *key;  // just the key directly
    int value;
    uint8 status;
} MyEntry;

#include "common/hashfn.h"
/* Generate hash table functions/types */
#define SH_PREFIX myhash
#define SH_KEY_TYPE const char *
#define SH_ELEMENT_TYPE MyEntry
#define SH_KEY key
#define SH_HASH_KEY(tb, key) string_hash((key), strlen(key))
#define SH_EQUAL(tb, a, b) (strcmp((a), (b)) == 0)
#define SH_SCOPE static inline  /* required */
#define SH_USE_STRINGS
#define SH_DECLARE
#include "lib/simplehash.h"


StringVector* compute_headers(QueryDesc* queryDesc);
void process_query(QueryDesc* queryDesc, StringVector* column_header, SubjectToStmt* subjectToStmt);
// bool reset_outputArrays = true;
// bool built_pointers = false;

// StringVector *attributes;


#endif							/* FAIRNESS_H */