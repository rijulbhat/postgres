/*-------------------------------------------------------------------------
 *
 * execMain.c
 *	  top level executor interface routines
 *
 * INTERFACE ROUTINES
 *	ExecutorStart()
 *	ExecutorRun()
 *	ExecutorFinish()
 *	ExecutorEnd()
 *
 *	These four procedures are the external interface to the executor.
 *	In each case, the query descriptor is required as an argument.
 *
 *	ExecutorStart must be called at the beginning of execution of any
 *	query plan and ExecutorEnd must always be called at the end of
 *	execution of a plan (unless it is aborted due to error).
 *
 *	ExecutorRun accepts direction and count arguments that specify whether
 *	the plan is to be executed forwards, backwards, and for how many tuples.
 *	In some cases ExecutorRun may be called multiple times to process all
 *	the tuples for a plan.  It is also acceptable to stop short of executing
 *	the whole plan (but only if it is a SELECT).
 *
 *	ExecutorFinish must be called after the final ExecutorRun call and
 *	before ExecutorEnd.  This can be omitted only in case of EXPLAIN,
 *	which should also omit ExecutorRun.
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/executor/execMain.c
 *
 *-------------------------------------------------------------------------
 */
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
#include "lib/rbtree.h"
#include "lib/pairingheap.h"
#include "lib/ilist.h"
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
/* Hooks for plugins to get control in ExecutorStart/Run/Finish/End */
ExecutorStart_hook_type ExecutorStart_hook = NULL;
ExecutorRun_hook_type ExecutorRun_hook = NULL;
ExecutorFinish_hook_type ExecutorFinish_hook = NULL;
ExecutorEnd_hook_type ExecutorEnd_hook = NULL;

/* Hook for plugin to get control in ExecCheckPermissions() */
ExecutorCheckPerms_hook_type ExecutorCheckPerms_hook = NULL;

/* decls for local routines only used within this module */
static void InitPlan(QueryDesc *queryDesc, int eflags);
static void CheckValidRowMarkRel(Relation rel, RowMarkType markType);
static void ExecPostprocessPlan(EState *estate);
static void ExecEndPlan(PlanState *planstate, EState *estate);
static void ExecutePlan(QueryDesc *queryDesc,
						CmdType operation,
						bool sendTuples,
						uint64 numberTuples,
						ScanDirection direction,
						DestReceiver *dest);
static bool ExecCheckOneRelPerms(RTEPermissionInfo *perminfo);
static bool ExecCheckPermissionsModified(Oid relOid, Oid userid,
										 Bitmapset *modifiedCols,
										 AclMode requiredPerms);
static void ExecCheckXactReadOnly(PlannedStmt *plannedstmt);
static void EvalPlanQualStart(EPQState *epqstate, Plan *planTree);
typedef struct Range {
	int start;
	int end;
	bool valid;
	double similarity;
} Range;
typedef struct IntRBNode
{
	RBTNode rbt_node;  /* Base RBTNode (must be first) */
	int key;           /* Integer key */
	int *values;         /* Integer value */
	int index;
	int start;
} IntRBNode;
/* Function prototype for createRange */
Range createRange(int st, int e);

Range createRange(int st, int e) {
	Range range;
	range.start = st;
	range.end = e;
	return range;
}
bool parse_string(const char *input, double *Wr, char *Cr, double *Wb, char *Cb, double *epsilon, char *column_name);
bool fairness_check(char **column_headers, CharPtrVector* vec, SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index);
int my_strcmp(const char *str1, const char *str2);
int get_attribute_index(char **column_headers, int natts, char *attribute);
void merge( int left, int mid, int right, int attr_index);
void merge_sort(int left, int right, int attr_index);
void buildingpointers(char **column_headers, char *attribute, int natts, SubjectToStmt* subjectToStmt);
void buildingpointers_w(char **column_headers, char *attribute, int natts, SubjectToStmt* subjectToStmt);
void insert_val(RBTree *rbt, int key, int value);
IntRBNode *get_node(RBTree *rbt, int key);
int jp(char **column_headers, char *sourceText, int natts, char *attribute,int curr,int color,char * dir,int *LJP,int* RJP,int * colorarray);
void initializestack(mystack* stack, char* name);
double jaccordsimilarity(Range r1, Range r2);
void pushstack(mystack* stack, int value);
void pop(mystack* stack);
float check();
char *print_table_names_from_query(QueryDesc *queryDesc);
void update_range(Range* range, int actualStart, int actualEnd, int outputStart, int outputEnd);
Range getrange(char **column_headers, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon);
Range multicolor_getrange(CharPtrVector* vec, char **column_headers, char* attribute, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon);
Range recursivedfs(Range originalrange,char **column_headers, CharPtrVector* vec, SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index);

int get_lower_value_index(int index, float value);
int get_upper_value_index(int index, float value);
static int num_tuples = 0;
static int MAX_ATTRS = 0;
static int sort_attr_index = 0;
void store_table_data(Oid relid, CharPtrVector *vec);
void pop(mystack* stack);
int top(mystack* stack);
int size(mystack* stack);
int *RJP, *LJP;
int *fwdPosPtr, *fwdNegPtr, *prevPosPtr, *prevNegPtr;
char ***outputArray;
static bool is_scan_node(Node *node);
// bool reset_outputArrays = true;
// bool built_pointers = false;
BoolVector *reset_outputArrays_vec;
BoolVector *built_pointers_vec;
PtrVector *table_pointers;
PtrVector *table_attributes;
PtrVector *table_LJP;
PtrVector *table_RJP;
// PtrVector *pointers;
// StringVector *attributes;
StringVector *tables;
bool reset_tables = true;

/* end of local decls */


/* ----------------------------------------attributes------------------------
 *		ExecutorStart
 *
 *		This routine must be called at the beginning of any execution of any
 *		query plan
 *
 * Takes a QueryDesc previously created by CreateQueryDesc (which is separate
 * only because some places use QueryDescs for utility commands).  The tupDesc
 * field of the QueryDesc is filled in to describe the tuples that will be
 * returned, and the internal fields (estate and planstate) are set up.
 *
 * eflags contains flag bits as described in executor.h.
 *
 * NB: the CurrentMemoryContext when this is called will become the parent
 * of the per-query context used for this Executor invocation.
 *
 * We provide a function hook variable that lets loadable plugins
 * get control when ExecutorStart is called.  Such a plugin would
 * normally call standard_ExecutorStart().
 *
 * Return value indicates if the plan has been initialized successfully so
 * that queryDesc->planstate contains a valid PlanState tree.  It may not
 * if the plan got invalidated during InitPlan().
 * ----------------------------------------------------------------
 */
bool
ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	bool		plan_valid;

	/*
	 * In some cases (e.g. an EXECUTE statement or an execute message with the
	 * extended query protocol) the query_id won't be reported, so do it now.
	 *
	 * Note that it's harmless to report the query_id multiple times, as the
	 * call will be ignored if the top level query_id has already been
	 * reported.
	 */
	pgstat_report_query_id(queryDesc->plannedstmt->queryId, false);

	if (ExecutorStart_hook)
		plan_valid = (*ExecutorStart_hook) (queryDesc, eflags);
	else
		plan_valid = standard_ExecutorStart(queryDesc, eflags);

	return plan_valid;
}

bool
standard_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	EState	   *estate;
	MemoryContext oldcontext;

	/* sanity checks: queryDesc must not be started already */
	Assert(queryDesc != NULL);
	Assert(queryDesc->estate == NULL);

	/* caller must ensure the query's snapshot is active */
	Assert(GetActiveSnapshot() == queryDesc->snapshot);

	/*
	 * If the transaction is read-only, we need to check if any writes are
	 * planned to non-temporary tables.  EXPLAIN is considered read-only.
	 *
	 * Don't allow writes in parallel mode.  Supporting UPDATE and DELETE
	 * would require (a) storing the combo CID hash in shared memory, rather
	 * than synchronizing it just once at the start of parallelism, and (b) an
	 * alternative to heap_update()'s reliance on xmax for mutual exclusion.
	 * INSERT may have no such troubles, but we forbid it to simplify the
	 * checks.
	 *
	 * We have lower-level defenses in CommandCounterIncrement and elsewhere
	 * against performing unsafe operations in parallel mode, but this gives a
	 * more user-friendly error message.
	 */
	if ((XactReadOnly || IsInParallelMode()) &&
		!(eflags & EXEC_FLAG_EXPLAIN_ONLY))
		ExecCheckXactReadOnly(queryDesc->plannedstmt);

	/*
	 * Build EState, switch into per-query memory context for startup.
	 */
	estate = CreateExecutorState();
	queryDesc->estate = estate;

	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	/*
	 * Fill in external parameters, if any, from queryDesc; and allocate
	 * workspace for internal parameters
	 */
	estate->es_param_list_info = queryDesc->params;

	if (queryDesc->plannedstmt->paramExecTypes != NIL)
	{
		int			nParamExec;

		nParamExec = list_length(queryDesc->plannedstmt->paramExecTypes);
		estate->es_param_exec_vals = (ParamExecData *)
			palloc0(nParamExec * sizeof(ParamExecData));
	}

	/* We now require all callers to provide sourceText */
	Assert(queryDesc->sourceText != NULL);
	estate->es_sourceText = queryDesc->sourceText;

	/*
	 * Fill in the query environment, if any, from queryDesc.
	 */
	estate->es_queryEnv = queryDesc->queryEnv;

	/*
	 * If non-read-only query, set the command ID to mark output tuples with
	 */
	switch (queryDesc->operation)
	{
		case CMD_SELECT:

			/*
			 * SELECT FOR [KEY] UPDATE/SHARE and modifying CTEs need to mark
			 * tuples
			 */
			if (queryDesc->plannedstmt->rowMarks != NIL ||
				queryDesc->plannedstmt->hasModifyingCTE)
				estate->es_output_cid = GetCurrentCommandId(true);

			/*
			 * A SELECT without modifying CTEs can't possibly queue triggers,
			 * so force skip-triggers mode. This is just a marginal efficiency
			 * hack, since AfterTriggerBeginQuery/AfterTriggerEndQuery aren't
			 * all that expensive, but we might as well do it.
			 */
			if (!queryDesc->plannedstmt->hasModifyingCTE)
				eflags |= EXEC_FLAG_SKIP_TRIGGERS;
			break;

		case CMD_INSERT:
		case CMD_DELETE:
		case CMD_UPDATE:
		case CMD_MERGE:
			estate->es_output_cid = GetCurrentCommandId(true);
			break;

		default:
			elog(ERROR, "unrecognized operation code: %d",
				 (int) queryDesc->operation);
			break;
	}

	/*
	 * Copy other important information into the EState
	 */
	estate->es_snapshot = RegisterSnapshot(queryDesc->snapshot);
	estate->es_crosscheck_snapshot = RegisterSnapshot(queryDesc->crosscheck_snapshot);
	estate->es_top_eflags = eflags;
	estate->es_instrument = queryDesc->instrument_options;
	estate->es_jit_flags = queryDesc->plannedstmt->jitFlags;

	/*
	 * Set up an AFTER-trigger statement context, unless told not to, or
	 * unless it's EXPLAIN-only mode (when ExecutorFinish won't be called).
	 */
	if (!(eflags & (EXEC_FLAG_SKIP_TRIGGERS | EXEC_FLAG_EXPLAIN_ONLY)))
		AfterTriggerBeginQuery();

	/*
	 * Initialize the plan state tree
	 */
	InitPlan(queryDesc, eflags);

	MemoryContextSwitchTo(oldcontext);

	return ExecPlanStillValid(queryDesc->estate);
}

/*
 * ExecutorStartCachedPlan
 *		Start execution for a given query in the CachedPlanSource, replanning
 *		if the plan is invalidated due to deferred locks taken during the
 *		plan's initialization
 *
 * This function handles cases where the CachedPlan given in queryDesc->cplan
 * might become invalid during the initialization of the plan given in
 * queryDesc->plannedstmt, particularly when prunable relations in it are
 * locked after performing initial pruning. If the locks invalidate the plan,
 * the function calls UpdateCachedPlan() to replan all queries in the
 * CachedPlan, and then retries initialization.
 *
 * The function repeats the process until ExecutorStart() successfully
 * initializes the plan, that is without the CachedPlan becoming invalid.
 */
void
ExecutorStartCachedPlan(QueryDesc *queryDesc, int eflags,
						CachedPlanSource *plansource,
						int query_index)
{
	if (unlikely(queryDesc->cplan == NULL))
		elog(ERROR, "ExecutorStartCachedPlan(): missing CachedPlan");
	if (unlikely(plansource == NULL))
		elog(ERROR, "ExecutorStartCachedPlan(): missing CachedPlanSource");

	/*
	 * Loop and retry with an updated plan until no further invalidation
	 * occurs.
	 */
	while (1)
	{
		if (!ExecutorStart(queryDesc, eflags))
		{
			/*
			 * Clean up the current execution state before creating the new
			 * plan to retry ExecutorStart().  Mark execution as aborted to
			 * ensure that AFTER trigger state is properly reset.
			 */
			queryDesc->estate->es_aborted = true;
			ExecutorEnd(queryDesc);

			/* Retry ExecutorStart() with an updated plan tree. */
			queryDesc->plannedstmt = UpdateCachedPlan(plansource, query_index,
													  queryDesc->queryEnv);
		}
		else

			/*
			 * Exit the loop if the plan is initialized successfully and no
			 * sinval messages were received that invalidated the CachedPlan.
			 */
			break;
	}
}

/* ----------------------------------------------------------------
 *		ExecutorRun
 *
 *		This is the main routine of the executor module. It accepts
 *		the query descriptor from the traffic cop and executes the
 *		query plan.
 *
 *		ExecutorStart must have been called already.
 *
 *		If direction is NoMovementScanDirection then nothing is done
 *		except to start up/shut down the destination.  Otherwise,
 *		we retrieve up to 'count' tuples in the specified direction.
 *
 *		Note: count = 0 is interpreted as no portal limit, i.e., run to
 *		completion.  Also note that the count limit is only applied to
 *		retrieved tuples, not for instance to those inserted/updated/deleted
 *		by a ModifyTable plan node.
 *
 *		There is no return value, but output tuples (if any) are sent to
 *		the destination receiver specified in the QueryDesc; and the number
 *		of tuples processed at the top level can be found in
 *		estate->es_processed.  The total number of tuples processed in all
 *		the ExecutorRun calls can be found in estate->es_total_processed.
 *
 *		We provide a function hook variable that lets loadable plugins
 *		get control when ExecutorRun is called.  Such a plugin would
 *		normally call standard_ExecutorRun().
 *
 * ----------------------------------------------------------------
 */
void
ExecutorRun(QueryDesc *queryDesc,
			ScanDirection direction, uint64 count)
{
	if (ExecutorRun_hook)
		(*ExecutorRun_hook) (queryDesc, direction, count);
	else
		standard_ExecutorRun(queryDesc, direction, count);
}

void
standard_ExecutorRun(QueryDesc *queryDesc,
					 ScanDirection direction, uint64 count)
{
	EState	   *estate;
	CmdType		operation;
	DestReceiver *dest;
	bool		sendTuples, fair;
	MemoryContext oldcontext;
	SubjectToStmt* subjectToStmt;
	StringVector *column_header;
	char *attribute;
	int epsilon;
	/* sanity checks */
	Assert(queryDesc != NULL);

	estate = queryDesc->estate;

	Assert(estate != NULL);
	Assert(!estate->es_aborted);
	Assert(!(estate->es_top_eflags & EXEC_FLAG_EXPLAIN_ONLY));

	/* caller must ensure the query's snapshot is active */
	Assert(GetActiveSnapshot() == estate->es_snapshot);

	/*
	 * Switch into per-query memory context
	 */
	// elog(INFO, "HI");
	 oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	subjectToStmt = (SubjectToStmt*) queryDesc->plannedstmt->subjectClause;
	// if (subjectToStmt != NULL){
	// 	elog(INFO, "Atribute: %s", subjectToStmt->attr);
	// }

	/* Allow instrumentation of Executor overall runtime */
	if (queryDesc->totaltime)
		InstrStartNode(queryDesc->totaltime);

	/*
	 * extract information from the query descriptor and the query feature.
	 */
	operation = queryDesc->operation;
	dest = queryDesc->dest;

	/*
	 * startup tuple receiver, if we will be emitting tuples
	 */
	estate->es_processed = 0;

	sendTuples = (operation == CMD_SELECT ||
				  queryDesc->plannedstmt->hasReturning);

	if (sendTuples){
		dest->rStartup(dest, operation, queryDesc->tupDesc);
		
		if (queryDesc->operation == CMD_SELECT)
		{
			// if (queryDesc->plannedstmt->subjectClause != NULL){
			// elog(INFO, "Header init");
			column_header = (StringVector *) malloc(sizeof(StringVector));
			string_vector_init(column_header);
			column_header->natts = queryDesc->tupDesc->natts;
			for (int i = 0; i < column_header->natts; i++)
			{
				Form_pg_attribute att = TupleDescAttr(queryDesc->tupDesc, i);
				attribute = (char *)malloc((strlen(NameStr(att->attname)) + 1) * sizeof(char));
				strcpy(attribute, NameStr(att->attname));
				string_vector_push(column_header, attribute);
			}	
		}
		
	}
	// elog(INFO,"HIIII");
	/*
	 * Run plan, unless direction is NoMovement.
	 *
	 * Note: pquery.c selects NoMovement if a prior call already reached
	 * end-of-data in the user-specified fetch direction.  This is important
	 * because various parts of the executor can misbehave if called again
	 * after reporting EOF.  For example, heapam.c would actually restart a
	 * heapscan and return all its data afresh.  There is also some doubt
	 * about whether a parallel plan would operate properly if an additional,
	 * necessarily non-parallel execution request occurs after completing a
	 * parallel execution.  (That case should work, but it's untested.)
	 */
	if (!ScanDirectionIsNoMovement(direction))
		ExecutePlan(queryDesc,
					operation,
					sendTuples,
					count,
					direction,
					dest);

	/*
	 * Update es_total_processed to keep track of the number of tuples
	 * processed across multiple ExecutorRun() calls.
	 */
	estate->es_total_processed += estate->es_processed;
	
	if(reset_tables){
		reset_tables = false;
		tables = (StringVector *) malloc(sizeof(StringVector));
		string_vector_init(tables);
		table_pointers = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_pointers);
		table_attributes = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_attributes);
		reset_outputArrays_vec = (BoolVector *) malloc(sizeof(BoolVector));
		bool_vector_init(reset_outputArrays_vec);
		built_pointers_vec = (BoolVector *) malloc(sizeof(BoolVector));
		bool_vector_init(built_pointers_vec);
		table_LJP = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_LJP);
		table_RJP = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_RJP);
	}

	char *table = print_table_names_from_query(queryDesc);
	int table_index = -1;
	// elog(INFO, "Table name: %s", table);
	if(table){
		table_index = string_vector_find(tables, table);
		// elog(INFO, "Table index: %d", table_index);
		if (table_index == -1){
			string_vector_push(tables, table);
			bool_vector_push(built_pointers_vec, false);
			bool_vector_push(reset_outputArrays_vec, true);
			PtrVector *new_table_pointers = (PtrVector *) malloc(sizeof(PtrVector));
			StringVector *new_table_attributes = (StringVector *) malloc(sizeof(StringVector));
			PtrVector *new_table_LJP = (PtrVector *) malloc(sizeof(PtrVector));
			PtrVector *new_table_RJP = (PtrVector *) malloc(sizeof(PtrVector));

			ptr_vector_init(new_table_pointers);
			string_vector_init(new_table_attributes);
			ptr_vector_init(new_table_LJP);
			ptr_vector_init(new_table_RJP);

			ptr_vector_push(table_pointers, (void *)new_table_pointers);
			ptr_vector_push(table_attributes, (void *)new_table_attributes);
			ptr_vector_push(table_LJP, (void *)new_table_LJP);
			ptr_vector_push(table_RJP, (void *)new_table_RJP);

			// elog(INFO, "Pushed table: %s", table);
			table_index = string_vector_find(tables, table);
			if (table_index == -1){
				elog(ERROR, "Table not found");
			}
		}
	}
	// elog(INFO,"HI2");
	if (queryDesc->operation == CMD_INSERT || queryDesc->operation == CMD_UPDATE || queryDesc->operation == CMD_MERGE || queryDesc->operation == CMD_DELETE){
		built_pointers_vec->data[table_index] = false;
		reset_outputArrays_vec->data[table_index] = true;
	}

	if (queryDesc->operation == CMD_SELECT && subjectToStmt != NULL){

		if(reset_outputArrays_vec->data[table_index]){
			table_pointers->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));
			table_attributes->data[table_index] = (StringVector *) malloc(sizeof(StringVector));
			ptr_vector_init(table_pointers->data[table_index]);
			string_vector_init(table_attributes->data[table_index]);
			reset_outputArrays_vec->data[table_index] = false;
		}

		Plan *plan;
		plan = queryDesc->planstate->plan;
		Oid relid = InvalidOid;

		PlanState *ps = queryDesc->planstate;

		// if (is_scan_node((Node *) ps))  // use the is_scan_node helper from before
		// {
		ScanState *scanstate = (ScanState *) ps;
		Scan *scan = (Scan *) scanstate->ps.plan;
	
		Index scanrelid = scan->scanrelid;
		CharPtrVector* vec;
		vec = (CharPtrVector *) malloc(sizeof(CharPtrVector));
		if (scanrelid > 0)
		{
			RangeTblEntry *rte = rt_fetch(scanrelid, queryDesc->estate->es_range_table);
			relid = rte->relid;
	
			
			char_ptr_vector_init(vec);
			store_table_data(relid, vec);
	
			// elog(INFO, "Cached data from relid: %u", relid);
		}
		else
		{
			elog(WARNING, "Invalid scanrelid (0)");
		}
		// }
		// else
		// {
		// 	elog(INFO, "Top-level planstate is not a scan node (type: %d)", nodeTag(ps));
		// }

		List *qualList;
		qualList = (List *) scan->plan.qual;
		ListCell *lc;
		int index, n_bounds, l_index, r_index, attr_index;
		char* attribute;
		float bounds[2];
		
		n_bounds = 0;
		
		if (list_length(subjectToStmt->attr_list) <= 1) {
			elog(ERROR, "SubjectToStmt attribute list must have more than one element.");
		}
		
		if (!qualList)
		{
			elog(ERROR, "missing where clause");
		}
		
		if (list_length(qualList) == 1)
		{
			elog(ERROR, "qualList has only one element, which is not allowed");
		}
		
		foreach(lc, qualList)
		{
			Node *node = (Node *) lfirst(lc);
			if (IsA(node, OpExpr))
			{
				OpExpr *opexpr = (OpExpr *) node;
				List *args = opexpr->args;
				if (list_length(args) == 2)
				{
					Expr *left = (Expr *) linitial(args);
					Expr *right = (Expr *) lsecond(args);
		
					if (IsA(left, Var) && IsA(right, Const))
					{
						Var *var = (Var *) left;
						Const *constant = (Const *) right;
						
						// elog(INFO, "Attribute number: %d", var->varattno);
						index = var->varattno - 1;
						attribute = column_header->data[index];
						
						// elog(INFO, "Constant value: %f", DatumGetFloat8(constant->constvalue));
						bounds[n_bounds++] = DatumGetFloat8(constant->constvalue);
						// Optional: get the operator itself
						// elog(INFO, "Operator OID: %u", opexpr->opno);
					}
				}
			}
		}

		num_tuples = vec->size;		
		outputArray = (char ***)malloc(vec->size * sizeof(char **));
					
		// elog(INFO, "Output vector size: %d", vec->size);
		for (int i = 0; i < vec->size; i++) {
			outputArray[i] = (char **)malloc(vec->natts * sizeof(char *));
			for (int j = 0; j < vec->natts; j++) {
				outputArray[i][j] = strdup(vec->data[i][j]);
			}
		}
		
		sort_attr_index = get_attribute_index(column_header->data, vec->natts, attribute);
		MAX_ATTRS = column_header->natts;
		Range output;

		if(list_length(subjectToStmt->attr_list) == 2){

			fair = fairness_check(column_header->data, vec, subjectToStmt, bounds[0], bounds[1], index);
			if(!fair){
				attr_index = string_vector_find(table_attributes->data[table_index], attribute);

				buildingpointers_w(column_header->data, attribute, vec->natts, subjectToStmt);
				// elog(INFO, "ohoi");
				
				epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
				// num_tuples = vec -> size;

				// if(!built_pointers_vec->data[table_index] || attr_index == -1){
				// 	outputArray = (char ***)malloc(vec->size * sizeof(char **));
					
				// 	// elog(INFO, "Output vector size: %d", vec->size);
				// 	for (int i = 0; i < vec->size; i++) {
				// 		outputArray[i] = (char **)malloc(vec->natts * sizeof(char *));
				// 		for (int j = 0; j < vec->natts; j++) {
				// 			outputArray[i][j] = strdup(vec->data[i][j]);
				// 		}
				// 	}
					
					
				// 	buildingpointers(column_header->data, attribute, vec->natts, subjectToStmt);

					
				// 	built_pointers_vec->data[table_index] = true;
				// 	// elog(INFO, "Built pointers");		
				// 	if (attr_index != -1) {
				// 		ptr_vector_replace(table_pointers->data[table_index], attr_index, (void *)outputArray);
				// 		ptr_vector_replace(table_LJP->data[table_index], attr_index, (void *)LJP);
				// 		ptr_vector_replace(table_RJP->data[table_index], attr_index, (void *)RJP);
				// 	} else {
				// 		ptr_vector_push(table_pointers->data[table_index], (void *)outputArray);
				// 		ptr_vector_push(table_LJP->data[table_index], (void *)LJP);
				// 		ptr_vector_push(table_RJP->data[table_index], (void *)RJP);
				// 		string_vector_push(table_attributes->data[table_index], attribute);
				// 		// elog(INFO, "pushed attribute: %s", attribute);
				// 	}
				// 	elog(INFO, "Pushed attribute: %s", attribute);
				// }
				
				// else{
				// 	outputArray = (char ***)ptr_vector_get(table_pointers->data[table_index], attr_index);
				// 	LJP = (int *)ptr_vector_get(table_LJP->data[table_index], attr_index);
				// 	RJP = (int *)ptr_vector_get(table_RJP->data[table_index], attr_index);
				// }
				// Print the outputArray for debugging
				// elog(INFO, "Printing outputArray:");
				// for (int i = 0; i < vec->size; i++) {
				// 	for (int j = 0; j < vec->natts; j++) {
				// 		if (outputArray[i][j] != NULL) {
				// 			elog(INFO, "outputArray[%d][%d] = %s", i, j, outputArray[i][j]);
				// 		} else {
				// 			elog(INFO, "outputArray[%d][%d] = NULL", i, j);
				// 		}
				// 	}
				// }
				// elog(INFO, "ohoi");
				l_index = get_upper_value_index(index, bounds[0]) + 1;
				r_index = get_lower_value_index(index, bounds[1]) + 1;
				elog(INFO, "l_index: %d, r_index: %d", l_index, r_index);
				output = getrange(column_header->data, vec->natts, subjectToStmt, l_index, r_index, epsilon);
				elog(INFO, "Fair range: [%d, %d]", output.start, output.end);
				if (output.end == num_tuples + 1)
				{
					output.end = num_tuples;
				}
				if (output.start == 0)
				{
					output.start = 1;
				}
				if(output.start == output.end){
					elog(INFO, "NO CORRECTED QUERY FOUND");
				}	
				else{
					elog(INFO, "CORRECTED QUERY:");
					elog(INFO, "SELECT * FROM %s WHERE %s BETWEEN %s AND %s", table, attribute, outputArray[output.start - 1][index], outputArray[output.end - 1][index]);
				}
			
			}
		}
		else if(list_length(subjectToStmt->attr_list) > 2){
			
			merge_sort(0, num_tuples - 1, sort_attr_index);
			l_index = get_upper_value_index(index, bounds[0]) + 1;
			r_index = get_lower_value_index(index, bounds[1]) + 1;
			epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
			output = multicolor_getrange(vec, column_header->data, attribute, vec->natts, subjectToStmt, l_index, r_index, epsilon);
			// elog(INFO, "Fair range: [%d, %d]", output.start, output.end);
			//%jalu writing this, do not hit
			Range originalrange = createRange(output.start, output.end);
			Range recursivedfsoutput = recursivedfs(originalrange, column_header->data, vec, subjectToStmt, bounds[0], bounds[1], index);
			elog(INFO, "Recursivedfs range: [%d, %d]", recursivedfsoutput.start, recursivedfsoutput.end);
			//%jalu ending this, do not blame 
			
			if (output.end == num_tuples + 1)
			{
				output.end = num_tuples;
			}
			if (output.start == 0)
			{
				output.start = 1;
			}
			if(output.start == output.end){
				elog(INFO, "NO CORRECTED QUERY FOUND");
			}	
			else{
				elog(INFO, "CORRECTED QUERY:");
				elog(INFO, "SELECT * FROM %s WHERE %s BETWEEN %s AND %s", table, attribute, outputArray[output.start - 1][index], outputArray[output.end - 1][index]);
			}

		}



		// elog(INFO, "Fair: %d", fair);
		// for (int i = 0; i < vec->size; i++) {
		// 	for (int j = 0; j < vec->natts; j++) {
		// 		if (j!=index) continue;
		// 		elog(INFO, "outputArray[%d][%d] = %s", i, j, outputArray[i][j]);
		// 	}
		// }
		
		// char_ptr_vector_free(vec);
		// free(vec);

		// string_vector_free(column_header);
		// free(column_header);
	}
	/*
	 * shutdown tuple receiver, if we started it
	 */
	if (sendTuples)
		dest->rShutdown(dest);

	if (queryDesc->totaltime)
		InstrStopNode(queryDesc->totaltime, estate->es_processed);
	
	MemoryContextSwitchTo(oldcontext);
}

/* ----------------------------------------------------------------
 *		ExecutorFinish
 *
 *		This routine must be called after the last ExecutorRun call.
 *		It performs cleanup such as firing AFTER triggers.  It is
 *		separate from ExecutorEnd because EXPLAIN ANALYZE needs to
 *		include these actions in the total runtime.
 *
 *		We provide a function hook variable that lets loadable plugins
 *		get control when ExecutorFinish is called.  Such a plugin would
 *		normally call standard_ExecutorFinish().
 *
 * ----------------------------------------------------------------
 */
void
ExecutorFinish(QueryDesc *queryDesc)
{
	if (ExecutorFinish_hook)
		(*ExecutorFinish_hook) (queryDesc);
	else
		standard_ExecutorFinish(queryDesc);
}

void
standard_ExecutorFinish(QueryDesc *queryDesc)
{
	EState	   *estate;
	MemoryContext oldcontext;

	/* sanity checks */
	Assert(queryDesc != NULL);

	estate = queryDesc->estate;

	Assert(estate != NULL);
	Assert(!(estate->es_top_eflags & EXEC_FLAG_EXPLAIN_ONLY));

	/*
	 * This should be run once and only once per Executor instance and never
	 * if the execution was aborted.
	 */
	Assert(!estate->es_finished && !estate->es_aborted);

	/* Switch into per-query memory context */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	/* Allow instrumentation of Executor overall runtime */
	if (queryDesc->totaltime)
		InstrStartNode(queryDesc->totaltime);

	/* Run ModifyTable nodes to completion */
	ExecPostprocessPlan(estate);

	/* Execute queued AFTER triggers, unless told not to */
	if (!(estate->es_top_eflags & EXEC_FLAG_SKIP_TRIGGERS))
		AfterTriggerEndQuery(estate);

	if (queryDesc->totaltime)
		InstrStopNode(queryDesc->totaltime, 0);

	MemoryContextSwitchTo(oldcontext);

	estate->es_finished = true;
}

/* ----------------------------------------------------------------
 *		ExecutorEnd
 *
 *		This routine must be called at the end of execution of any
 *		query plan
 *
 *		We provide a function hook variable that lets loadable plugins
 *		get control when ExecutorEnd is called.  Such a plugin would
 *		normally call standard_ExecutorEnd().
 *
 * ----------------------------------------------------------------
 */
void
ExecutorEnd(QueryDesc *queryDesc)
{
	if (ExecutorEnd_hook)
		(*ExecutorEnd_hook) (queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

void
standard_ExecutorEnd(QueryDesc *queryDesc)
{
	EState	   *estate;
	MemoryContext oldcontext;

	/* sanity checks */
	Assert(queryDesc != NULL);

	estate = queryDesc->estate;

	Assert(estate != NULL);

	if (estate->es_parallel_workers_to_launch > 0)
		pgstat_update_parallel_workers_stats((PgStat_Counter) estate->es_parallel_workers_to_launch,
											 (PgStat_Counter) estate->es_parallel_workers_launched);

	/*
	 * Check that ExecutorFinish was called, unless in EXPLAIN-only mode or if
	 * execution was aborted.
	 */
	Assert(estate->es_finished || estate->es_aborted ||
		   (estate->es_top_eflags & EXEC_FLAG_EXPLAIN_ONLY));

	/*
	 * Switch into per-query memory context to run ExecEndPlan
	 */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	ExecEndPlan(queryDesc->planstate, estate);

	/* do away with our snapshots */
	UnregisterSnapshot(estate->es_snapshot);
	UnregisterSnapshot(estate->es_crosscheck_snapshot);

	/*
	 * Reset AFTER trigger module if the query execution was aborted.
	 */
	if (estate->es_aborted &&
		!(estate->es_top_eflags &
		  (EXEC_FLAG_SKIP_TRIGGERS | EXEC_FLAG_EXPLAIN_ONLY)))
		AfterTriggerAbortQuery();

	/*
	 * Must switch out of context before destroying it
	 */
	MemoryContextSwitchTo(oldcontext);

	/*
	 * Release EState and per-query memory context.  This should release
	 * everything the executor has allocated.
	 */
	FreeExecutorState(estate);

	/* Reset queryDesc fields that no longer point to anything */
	queryDesc->tupDesc = NULL;
	queryDesc->estate = NULL;
	queryDesc->planstate = NULL;
	queryDesc->totaltime = NULL;
}

/* ----------------------------------------------------------------
 *		ExecutorRewind
 *
 *		This routine may be called on an open queryDesc to rewind it
 *		to the start.
 * ----------------------------------------------------------------
 */
void
ExecutorRewind(QueryDesc *queryDesc)
{
	EState	   *estate;
	MemoryContext oldcontext;

	/* sanity checks */
	Assert(queryDesc != NULL);

	estate = queryDesc->estate;

	Assert(estate != NULL);

	/* It's probably not sensible to rescan updating queries */
	Assert(queryDesc->operation == CMD_SELECT);

	/*
	 * Switch into per-query memory context
	 */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	/*
	 * rescan plan
	 */
	ExecReScan(queryDesc->planstate);

	MemoryContextSwitchTo(oldcontext);
}


/*
 * ExecCheckPermissions
 *		Check access permissions of relations mentioned in a query
 *
 * Returns true if permissions are adequate.  Otherwise, throws an appropriate
 * error if ereport_on_violation is true, or simply returns false otherwise.
 *
 * Note that this does NOT address row-level security policies (aka: RLS).  If
 * rows will be returned to the user as a result of this permission check
 * passing, then RLS also needs to be consulted (and check_enable_rls()).
 *
 * See rewrite/rowsecurity.c.
 *
 * NB: rangeTable is no longer used by us, but kept around for the hooks that
 * might still want to look at the RTEs.
 */
bool
ExecCheckPermissions(List *rangeTable, List *rteperminfos,
					 bool ereport_on_violation)
{
	ListCell   *l;
	bool		result = true;

#ifdef USE_ASSERT_CHECKING
	Bitmapset  *indexset = NULL;

	/* Check that rteperminfos is consistent with rangeTable */
	foreach(l, rangeTable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, l);

		if (rte->perminfoindex != 0)
		{
			/* Sanity checks */

			/*
			 * Only relation RTEs and subquery RTEs that were once relation
			 * RTEs (views) have their perminfoindex set.
			 */
			Assert(rte->rtekind == RTE_RELATION ||
				   (rte->rtekind == RTE_SUBQUERY &&
					rte->relkind == RELKIND_VIEW));

			/*
			 * Ensure that we have at least an AccessShareLock on relations
			 * whose permissions need to be checked.
			 *
			 * Skip this check in a parallel worker because locks won't be
			 * taken until ExecInitNode() performs plan initialization.
			 *
			 * XXX: ExecCheckPermissions() in a parallel worker may be
			 * redundant with the checks done in the leader process, so this
			 * should be reviewed to ensure it’s necessary.
			 */
			Assert(IsParallelWorker() ||
				   CheckRelationOidLockedByMe(rte->relid, AccessShareLock,
											  true));

			(void) getRTEPermissionInfo(rteperminfos, rte);
			/* Many-to-one mapping not allowed */
			Assert(!bms_is_member(rte->perminfoindex, indexset));
			indexset = bms_add_member(indexset, rte->perminfoindex);
		}
	}

	/* All rteperminfos are referenced */
	Assert(bms_num_members(indexset) == list_length(rteperminfos));
#endif

	foreach(l, rteperminfos)
	{
		RTEPermissionInfo *perminfo = lfirst_node(RTEPermissionInfo, l);

		Assert(OidIsValid(perminfo->relid));
		result = ExecCheckOneRelPerms(perminfo);
		if (!result)
		{
			if (ereport_on_violation)
				aclcheck_error(ACLCHECK_NO_PRIV,
							   get_relkind_objtype(get_rel_relkind(perminfo->relid)),
							   get_rel_name(perminfo->relid));
			return false;
		}
	}

	if (ExecutorCheckPerms_hook)
		result = (*ExecutorCheckPerms_hook) (rangeTable, rteperminfos,
											 ereport_on_violation);
	return result;
}

/*
 * ExecCheckOneRelPerms
 *		Check access permissions for a single relation.
 */
static bool
ExecCheckOneRelPerms(RTEPermissionInfo *perminfo)
{
	AclMode		requiredPerms;
	AclMode		relPerms;
	AclMode		remainingPerms;
	Oid			userid;
	Oid			relOid = perminfo->relid;

	requiredPerms = perminfo->requiredPerms;
	Assert(requiredPerms != 0);

	/*
	 * userid to check as: current user unless we have a setuid indication.
	 *
	 * Note: GetUserId() is presently fast enough that there's no harm in
	 * calling it separately for each relation.  If that stops being true, we
	 * could call it once in ExecCheckPermissions and pass the userid down
	 * from there.  But for now, no need for the extra clutter.
	 */
	userid = OidIsValid(perminfo->checkAsUser) ?
		perminfo->checkAsUser : GetUserId();

	/*
	 * We must have *all* the requiredPerms bits, but some of the bits can be
	 * satisfied from column-level rather than relation-level permissions.
	 * First, remove any bits that are satisfied by relation permissions.
	 */
	relPerms = pg_class_aclmask(relOid, userid, requiredPerms, ACLMASK_ALL);
	remainingPerms = requiredPerms & ~relPerms;
	if (remainingPerms != 0)
	{
		int			col = -1;

		/*
		 * If we lack any permissions that exist only as relation permissions,
		 * we can fail straight away.
		 */
		if (remainingPerms & ~(ACL_SELECT | ACL_INSERT | ACL_UPDATE))
			return false;

		/*
		 * Check to see if we have the needed privileges at column level.
		 *
		 * Note: failures just report a table-level error; it would be nicer
		 * to report a column-level error if we have some but not all of the
		 * column privileges.
		 */
		if (remainingPerms & ACL_SELECT)
		{
			/*
			 * When the query doesn't explicitly reference any columns (for
			 * example, SELECT COUNT(*) FROM table), allow the query if we
			 * have SELECT on any column of the rel, as per SQL spec.
			 */
			if (bms_is_empty(perminfo->selectedCols))
			{
				if (pg_attribute_aclcheck_all(relOid, userid, ACL_SELECT,
											  ACLMASK_ANY) != ACLCHECK_OK)
					return false;
			}

			while ((col = bms_next_member(perminfo->selectedCols, col)) >= 0)
			{
				/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
				AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

				if (attno == InvalidAttrNumber)
				{
					/* Whole-row reference, must have priv on all cols */
					if (pg_attribute_aclcheck_all(relOid, userid, ACL_SELECT,
												  ACLMASK_ALL) != ACLCHECK_OK)
						return false;
				}
				else
				{
					if (pg_attribute_aclcheck(relOid, attno, userid,
											  ACL_SELECT) != ACLCHECK_OK)
						return false;
				}
			}
		}

		/*
		 * Basically the same for the mod columns, for both INSERT and UPDATE
		 * privilege as specified by remainingPerms.
		 */
		if (remainingPerms & ACL_INSERT &&
			!ExecCheckPermissionsModified(relOid,
										  userid,
										  perminfo->insertedCols,
										  ACL_INSERT))
			return false;

		if (remainingPerms & ACL_UPDATE &&
			!ExecCheckPermissionsModified(relOid,
										  userid,
										  perminfo->updatedCols,
										  ACL_UPDATE))
			return false;
	}
	return true;
}

/*
 * ExecCheckPermissionsModified
 *		Check INSERT or UPDATE access permissions for a single relation (these
 *		are processed uniformly).
 */
static bool
ExecCheckPermissionsModified(Oid relOid, Oid userid, Bitmapset *modifiedCols,
							 AclMode requiredPerms)
{
	int			col = -1;

	/*
	 * When the query doesn't explicitly update any columns, allow the query
	 * if we have permission on any column of the rel.  This is to handle
	 * SELECT FOR UPDATE as well as possible corner cases in UPDATE.
	 */
	if (bms_is_empty(modifiedCols))
	{
		if (pg_attribute_aclcheck_all(relOid, userid, requiredPerms,
									  ACLMASK_ANY) != ACLCHECK_OK)
			return false;
	}

	while ((col = bms_next_member(modifiedCols, col)) >= 0)
	{
		/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
		AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

		if (attno == InvalidAttrNumber)
		{
			/* whole-row reference can't happen here */
			elog(ERROR, "whole-row update is not implemented");
		}
		else
		{
			if (pg_attribute_aclcheck(relOid, attno, userid,
									  requiredPerms) != ACLCHECK_OK)
				return false;
		}
	}
	return true;
}

/*
 * Check that the query does not imply any writes to non-temp tables;
 * unless we're in parallel mode, in which case don't even allow writes
 * to temp tables.
 *
 * Note: in a Hot Standby this would need to reject writes to temp
 * tables just as we do in parallel mode; but an HS standby can't have created
 * any temp tables in the first place, so no need to check that.
 */
static void
ExecCheckXactReadOnly(PlannedStmt *plannedstmt)
{
	ListCell   *l;

	/*
	 * Fail if write permissions are requested in parallel mode for table
	 * (temp or non-temp), otherwise fail for any non-temp table.
	 */
	foreach(l, plannedstmt->permInfos)
	{
		RTEPermissionInfo *perminfo = lfirst_node(RTEPermissionInfo, l);

		if ((perminfo->requiredPerms & (~ACL_SELECT)) == 0)
			continue;

		if (isTempNamespace(get_rel_namespace(perminfo->relid)))
			continue;

		PreventCommandIfReadOnly(CreateCommandName((Node *) plannedstmt));
	}

	if (plannedstmt->commandType != CMD_SELECT || plannedstmt->hasModifyingCTE)
		PreventCommandIfParallelMode(CreateCommandName((Node *) plannedstmt));
}


/* ----------------------------------------------------------------
 *		InitPlan
 *
 *		Initializes the query plan: open files, allocate storage
 *		and start up the rule manager
 *
 *		If the plan originates from a CachedPlan (given in queryDesc->cplan),
 *		it can become invalid during runtime "initial" pruning when the
 *		remaining set of locks is taken.  The function returns early in that
 *		case without initializing the plan, and the caller is expected to
 *		retry with a new valid plan.
 * ----------------------------------------------------------------
 */
static void
InitPlan(QueryDesc *queryDesc, int eflags)
{
	CmdType		operation = queryDesc->operation;
	PlannedStmt *plannedstmt = queryDesc->plannedstmt;
	CachedPlan *cachedplan = queryDesc->cplan;
	Plan	   *plan = plannedstmt->planTree;
	List	   *rangeTable = plannedstmt->rtable;
	EState	   *estate = queryDesc->estate;
	PlanState  *planstate;
	TupleDesc	tupType;
	ListCell   *l;
	int			i;

	/*
	 * Do permissions checks
	 */
	ExecCheckPermissions(rangeTable, plannedstmt->permInfos, true);

	/*
	 * initialize the node's execution state
	 */
	ExecInitRangeTable(estate, rangeTable, plannedstmt->permInfos,
					   bms_copy(plannedstmt->unprunableRelids));

	estate->es_plannedstmt = plannedstmt;
	estate->es_cachedplan = cachedplan;
	estate->es_part_prune_infos = plannedstmt->partPruneInfos;

	/*
	 * Perform runtime "initial" pruning to identify which child subplans,
	 * corresponding to the children of plan nodes that contain
	 * PartitionPruneInfo such as Append, will not be executed. The results,
	 * which are bitmapsets of indexes of the child subplans that will be
	 * executed, are saved in es_part_prune_results.  These results correspond
	 * to each PartitionPruneInfo entry, and the es_part_prune_results list is
	 * parallel to es_part_prune_infos.
	 */
	ExecDoInitialPruning(estate);

	if (!ExecPlanStillValid(estate))
		return;

	/*
	 * Next, build the ExecRowMark array from the PlanRowMark(s), if any.
	 */
	if (plannedstmt->rowMarks)
	{
		estate->es_rowmarks = (ExecRowMark **)
			palloc0(estate->es_range_table_size * sizeof(ExecRowMark *));
		foreach(l, plannedstmt->rowMarks)
		{
			PlanRowMark *rc = (PlanRowMark *) lfirst(l);
			Oid			relid;
			Relation	relation;
			ExecRowMark *erm;

			/*
			 * Ignore "parent" rowmarks, because they are irrelevant at
			 * runtime.  Also ignore the rowmarks belonging to child tables
			 * that have been pruned in ExecDoInitialPruning().
			 */
			if (rc->isParent ||
				!bms_is_member(rc->rti, estate->es_unpruned_relids))
				continue;

			/* get relation's OID (will produce InvalidOid if subquery) */
			relid = exec_rt_fetch(rc->rti, estate)->relid;

			/* open relation, if we need to access it for this mark type */
			switch (rc->markType)
			{
				case ROW_MARK_EXCLUSIVE:
				case ROW_MARK_NOKEYEXCLUSIVE:
				case ROW_MARK_SHARE:
				case ROW_MARK_KEYSHARE:
				case ROW_MARK_REFERENCE:
					relation = ExecGetRangeTableRelation(estate, rc->rti);
					break;
				case ROW_MARK_COPY:
					/* no physical table access is required */
					relation = NULL;
					break;
				default:
					elog(ERROR, "unrecognized markType: %d", rc->markType);
					relation = NULL;	/* keep compiler quiet */
					break;
			}

			/* Check that relation is a legal target for marking */
			if (relation)
				CheckValidRowMarkRel(relation, rc->markType);

			erm = (ExecRowMark *) palloc(sizeof(ExecRowMark));
			erm->relation = relation;
			erm->relid = relid;
			erm->rti = rc->rti;
			erm->prti = rc->prti;
			erm->rowmarkId = rc->rowmarkId;
			erm->markType = rc->markType;
			erm->strength = rc->strength;
			erm->waitPolicy = rc->waitPolicy;
			erm->ermActive = false;
			ItemPointerSetInvalid(&(erm->curCtid));
			erm->ermExtra = NULL;

			Assert(erm->rti > 0 && erm->rti <= estate->es_range_table_size &&
				   estate->es_rowmarks[erm->rti - 1] == NULL);

			estate->es_rowmarks[erm->rti - 1] = erm;
		}
	}

	/*
	 * Initialize the executor's tuple table to empty.
	 */
	estate->es_tupleTable = NIL;

	/* signal that this EState is not used for EPQ */
	estate->es_epq_active = NULL;

	/*
	 * Initialize private state information for each SubPlan.  We must do this
	 * before running ExecInitNode on the main query tree, since
	 * ExecInitSubPlan expects to be able to find these entries.
	 */
	Assert(estate->es_subplanstates == NIL);
	i = 1;						/* subplan indices count from 1 */
	foreach(l, plannedstmt->subplans)
	{
		Plan	   *subplan = (Plan *) lfirst(l);
		PlanState  *subplanstate;
		int			sp_eflags;

		/*
		 * A subplan will never need to do BACKWARD scan nor MARK/RESTORE. If
		 * it is a parameterless subplan (not initplan), we suggest that it be
		 * prepared to handle REWIND efficiently; otherwise there is no need.
		 */
		sp_eflags = eflags
			& ~(EXEC_FLAG_REWIND | EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK);
		if (bms_is_member(i, plannedstmt->rewindPlanIDs))
			sp_eflags |= EXEC_FLAG_REWIND;

		subplanstate = ExecInitNode(subplan, estate, sp_eflags);

		estate->es_subplanstates = lappend(estate->es_subplanstates,
										   subplanstate);

		i++;
	}

	/*
	 * Initialize the private state information for all the nodes in the query
	 * tree.  This opens files, allocates storage and leaves us ready to start
	 * processing tuples.
	 */
	planstate = ExecInitNode(plan, estate, eflags);

	/*
	 * Get the tuple descriptor describing the type of tuples to return.
	 */
	tupType = ExecGetResultType(planstate);

	/*
	 * Initialize the junk filter if needed.  SELECT queries need a filter if
	 * there are any junk attrs in the top-level tlist.
	 */
	if (operation == CMD_SELECT)
	{
		bool		junk_filter_needed = false;
		ListCell   *tlist;

		foreach(tlist, plan->targetlist)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(tlist);

			if (tle->resjunk)
			{
				junk_filter_needed = true;
				break;
			}
		}

		if (junk_filter_needed)
		{
			JunkFilter *j;
			TupleTableSlot *slot;

			slot = ExecInitExtraTupleSlot(estate, NULL, &TTSOpsVirtual);
			j = ExecInitJunkFilter(planstate->plan->targetlist,
								   slot);
			estate->es_junkFilter = j;

			/* Want to return the cleaned tuple type */
			tupType = j->jf_cleanTupType;
		}
	}

	queryDesc->tupDesc = tupType;
	queryDesc->planstate = planstate;
}

/*
 * Check that a proposed result relation is a legal target for the operation
 *
 * Generally the parser and/or planner should have noticed any such mistake
 * already, but let's make sure.
 *
 * For MERGE, mergeActions is the list of actions that may be performed.  The
 * result relation is required to support every action, regardless of whether
 * or not they are all executed.
 *
 * Note: when changing this function, you probably also need to look at
 * CheckValidRowMarkRel.
 */
void
CheckValidResultRel(ResultRelInfo *resultRelInfo, CmdType operation,
					List *mergeActions)
{
	Relation	resultRel = resultRelInfo->ri_RelationDesc;
	FdwRoutine *fdwroutine;

	/* Expect a fully-formed ResultRelInfo from InitResultRelInfo(). */
	Assert(resultRelInfo->ri_needLockTagTuple ==
		   IsInplaceUpdateRelation(resultRel));

	switch (resultRel->rd_rel->relkind)
	{
		case RELKIND_RELATION:
		case RELKIND_PARTITIONED_TABLE:
			CheckCmdReplicaIdentity(resultRel, operation);
			break;
		case RELKIND_SEQUENCE:
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot change sequence \"%s\"",
							RelationGetRelationName(resultRel))));
			break;
		case RELKIND_TOASTVALUE:
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot change TOAST relation \"%s\"",
							RelationGetRelationName(resultRel))));
			break;
		case RELKIND_VIEW:

			/*
			 * Okay only if there's a suitable INSTEAD OF trigger.  Otherwise,
			 * complain, but omit errdetail because we haven't got the
			 * information handy (and given that it really shouldn't happen,
			 * it's not worth great exertion to get).
			 */
			if (!view_has_instead_trigger(resultRel, operation, mergeActions))
				error_view_not_updatable(resultRel, operation, mergeActions,
										 NULL);
			break;
		case RELKIND_MATVIEW:
			if (!MatViewIncrementalMaintenanceIsEnabled())
				ereport(ERROR,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
						 errmsg("cannot change materialized view \"%s\"",
								RelationGetRelationName(resultRel))));
			break;
		case RELKIND_FOREIGN_TABLE:
			/* Okay only if the FDW supports it */
			fdwroutine = resultRelInfo->ri_FdwRoutine;
			switch (operation)
			{
				case CMD_INSERT:
					if (fdwroutine->ExecForeignInsert == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("cannot insert into foreign table \"%s\"",
										RelationGetRelationName(resultRel))));
					if (fdwroutine->IsForeignRelUpdatable != NULL &&
						(fdwroutine->IsForeignRelUpdatable(resultRel) & (1 << CMD_INSERT)) == 0)
						ereport(ERROR,
								(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
								 errmsg("foreign table \"%s\" does not allow inserts",
										RelationGetRelationName(resultRel))));
					break;
				case CMD_UPDATE:
					if (fdwroutine->ExecForeignUpdate == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("cannot update foreign table \"%s\"",
										RelationGetRelationName(resultRel))));
					if (fdwroutine->IsForeignRelUpdatable != NULL &&
						(fdwroutine->IsForeignRelUpdatable(resultRel) & (1 << CMD_UPDATE)) == 0)
						ereport(ERROR,
								(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
								 errmsg("foreign table \"%s\" does not allow updates",
										RelationGetRelationName(resultRel))));
					break;
				case CMD_DELETE:
					if (fdwroutine->ExecForeignDelete == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("cannot delete from foreign table \"%s\"",
										RelationGetRelationName(resultRel))));
					if (fdwroutine->IsForeignRelUpdatable != NULL &&
						(fdwroutine->IsForeignRelUpdatable(resultRel) & (1 << CMD_DELETE)) == 0)
						ereport(ERROR,
								(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
								 errmsg("foreign table \"%s\" does not allow deletes",
										RelationGetRelationName(resultRel))));
					break;
				default:
					elog(ERROR, "unrecognized CmdType: %d", (int) operation);
					break;
			}
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot change relation \"%s\"",
							RelationGetRelationName(resultRel))));
			break;
	}
}

/*
 * Check that a proposed rowmark target relation is a legal target
 *
 * In most cases parser and/or planner should have noticed this already, but
 * they don't cover all cases.
 */
static void
CheckValidRowMarkRel(Relation rel, RowMarkType markType)
{
	FdwRoutine *fdwroutine;

	switch (rel->rd_rel->relkind)
	{
		case RELKIND_RELATION:
		case RELKIND_PARTITIONED_TABLE:
			/* OK */
			break;
		case RELKIND_SEQUENCE:
			/* Must disallow this because we don't vacuum sequences */
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot lock rows in sequence \"%s\"",
							RelationGetRelationName(rel))));
			break;
		case RELKIND_TOASTVALUE:
			/* We could allow this, but there seems no good reason to */
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot lock rows in TOAST relation \"%s\"",
							RelationGetRelationName(rel))));
			break;
		case RELKIND_VIEW:
			/* Should not get here; planner should have expanded the view */
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot lock rows in view \"%s\"",
							RelationGetRelationName(rel))));
			break;
		case RELKIND_MATVIEW:
			/* Allow referencing a matview, but not actual locking clauses */
			if (markType != ROW_MARK_REFERENCE)
				ereport(ERROR,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
						 errmsg("cannot lock rows in materialized view \"%s\"",
								RelationGetRelationName(rel))));
			break;
		case RELKIND_FOREIGN_TABLE:
			/* Okay only if the FDW supports it */
			fdwroutine = GetFdwRoutineForRelation(rel, false);
			if (fdwroutine->RefetchForeignRow == NULL)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("cannot lock rows in foreign table \"%s\"",
								RelationGetRelationName(rel))));
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot lock rows in relation \"%s\"",
							RelationGetRelationName(rel))));
			break;
	}
}

/*
 * Initialize ResultRelInfo data for one result relation
 *
 * Caution: before Postgres 9.1, this function included the relkind checking
 * that's now in CheckValidResultRel, and it also did ExecOpenIndices if
 * appropriate.  Be sure callers cover those needs.
 */
void
InitResultRelInfo(ResultRelInfo *resultRelInfo,
				  Relation resultRelationDesc,
				  Index resultRelationIndex,
				  ResultRelInfo *partition_root_rri,
				  int instrument_options)
{
	MemSet(resultRelInfo, 0, sizeof(ResultRelInfo));
	resultRelInfo->type = T_ResultRelInfo;
	resultRelInfo->ri_RangeTableIndex = resultRelationIndex;
	resultRelInfo->ri_RelationDesc = resultRelationDesc;
	resultRelInfo->ri_NumIndices = 0;
	resultRelInfo->ri_IndexRelationDescs = NULL;
	resultRelInfo->ri_IndexRelationInfo = NULL;
	resultRelInfo->ri_needLockTagTuple =
		IsInplaceUpdateRelation(resultRelationDesc);
	/* make a copy so as not to depend on relcache info not changing... */
	resultRelInfo->ri_TrigDesc = CopyTriggerDesc(resultRelationDesc->trigdesc);
	if (resultRelInfo->ri_TrigDesc)
	{
		int			n = resultRelInfo->ri_TrigDesc->numtriggers;

		resultRelInfo->ri_TrigFunctions = (FmgrInfo *)
			palloc0(n * sizeof(FmgrInfo));
		resultRelInfo->ri_TrigWhenExprs = (ExprState **)
			palloc0(n * sizeof(ExprState *));
		if (instrument_options)
			resultRelInfo->ri_TrigInstrument = InstrAlloc(n, instrument_options, false);
	}
	else
	{
		resultRelInfo->ri_TrigFunctions = NULL;
		resultRelInfo->ri_TrigWhenExprs = NULL;
		resultRelInfo->ri_TrigInstrument = NULL;
	}
	if (resultRelationDesc->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
		resultRelInfo->ri_FdwRoutine = GetFdwRoutineForRelation(resultRelationDesc, true);
	else
		resultRelInfo->ri_FdwRoutine = NULL;

	/* The following fields are set later if needed */
	resultRelInfo->ri_RowIdAttNo = 0;
	resultRelInfo->ri_extraUpdatedCols = NULL;
	resultRelInfo->ri_projectNew = NULL;
	resultRelInfo->ri_newTupleSlot = NULL;
	resultRelInfo->ri_oldTupleSlot = NULL;
	resultRelInfo->ri_projectNewInfoValid = false;
	resultRelInfo->ri_FdwState = NULL;
	resultRelInfo->ri_usesFdwDirectModify = false;
	resultRelInfo->ri_ConstraintExprs = NULL;
	resultRelInfo->ri_GeneratedExprsI = NULL;
	resultRelInfo->ri_GeneratedExprsU = NULL;
	resultRelInfo->ri_projectReturning = NULL;
	resultRelInfo->ri_onConflictArbiterIndexes = NIL;
	resultRelInfo->ri_onConflict = NULL;
	resultRelInfo->ri_ReturningSlot = NULL;
	resultRelInfo->ri_TrigOldSlot = NULL;
	resultRelInfo->ri_TrigNewSlot = NULL;
	resultRelInfo->ri_AllNullSlot = NULL;
	resultRelInfo->ri_MergeActions[MERGE_WHEN_MATCHED] = NIL;
	resultRelInfo->ri_MergeActions[MERGE_WHEN_NOT_MATCHED_BY_SOURCE] = NIL;
	resultRelInfo->ri_MergeActions[MERGE_WHEN_NOT_MATCHED_BY_TARGET] = NIL;
	resultRelInfo->ri_MergeJoinCondition = NULL;

	/*
	 * Only ExecInitPartitionInfo() and ExecInitPartitionDispatchInfo() pass
	 * non-NULL partition_root_rri.  For child relations that are part of the
	 * initial query rather than being dynamically added by tuple routing,
	 * this field is filled in ExecInitModifyTable().
	 */
	resultRelInfo->ri_RootResultRelInfo = partition_root_rri;
	/* Set by ExecGetRootToChildMap */
	resultRelInfo->ri_RootToChildMap = NULL;
	resultRelInfo->ri_RootToChildMapValid = false;
	/* Set by ExecInitRoutingInfo */
	resultRelInfo->ri_PartitionTupleSlot = NULL;
	resultRelInfo->ri_ChildToRootMap = NULL;
	resultRelInfo->ri_ChildToRootMapValid = false;
	resultRelInfo->ri_CopyMultiInsertBuffer = NULL;
}

/*
 * ExecGetTriggerResultRel
 *		Get a ResultRelInfo for a trigger target relation.
 *
 * Most of the time, triggers are fired on one of the result relations of the
 * query, and so we can just return a member of the es_result_relations array,
 * or the es_tuple_routing_result_relations list (if any). (Note: in self-join
 * situations there might be multiple members with the same OID; if so it
 * doesn't matter which one we pick.)
 *
 * However, it is sometimes necessary to fire triggers on other relations;
 * this happens mainly when an RI update trigger queues additional triggers
 * on other relations, which will be processed in the context of the outer
 * query.  For efficiency's sake, we want to have a ResultRelInfo for those
 * triggers too; that can avoid repeated re-opening of the relation.  (It
 * also provides a way for EXPLAIN ANALYZE to report the runtimes of such
 * triggers.)  So we make additional ResultRelInfo's as needed, and save them
 * in es_trig_target_relations.
 */
ResultRelInfo *
ExecGetTriggerResultRel(EState *estate, Oid relid,
						ResultRelInfo *rootRelInfo)
{
	ResultRelInfo *rInfo;
	ListCell   *l;
	Relation	rel;
	MemoryContext oldcontext;

	/* Search through the query result relations */
	foreach(l, estate->es_opened_result_relations)
	{
		rInfo = lfirst(l);
		if (RelationGetRelid(rInfo->ri_RelationDesc) == relid)
			return rInfo;
	}

	/*
	 * Search through the result relations that were created during tuple
	 * routing, if any.
	 */
	foreach(l, estate->es_tuple_routing_result_relations)
	{
		rInfo = (ResultRelInfo *) lfirst(l);
		if (RelationGetRelid(rInfo->ri_RelationDesc) == relid)
			return rInfo;
	}

	/* Nope, but maybe we already made an extra ResultRelInfo for it */
	foreach(l, estate->es_trig_target_relations)
	{
		rInfo = (ResultRelInfo *) lfirst(l);
		if (RelationGetRelid(rInfo->ri_RelationDesc) == relid)
			return rInfo;
	}
	/* Nope, so we need a new one */

	/*
	 * Open the target relation's relcache entry.  We assume that an
	 * appropriate lock is still held by the backend from whenever the trigger
	 * event got queued, so we need take no new lock here.  Also, we need not
	 * recheck the relkind, so no need for CheckValidResultRel.
	 */
	rel = table_open(relid, NoLock);

	/*
	 * Make the new entry in the right context.
	 */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);
	rInfo = makeNode(ResultRelInfo);
	InitResultRelInfo(rInfo,
					  rel,
					  0,		/* dummy rangetable index */
					  rootRelInfo,
					  estate->es_instrument);
	estate->es_trig_target_relations =
		lappend(estate->es_trig_target_relations, rInfo);
	MemoryContextSwitchTo(oldcontext);

	/*
	 * Currently, we don't need any index information in ResultRelInfos used
	 * only for triggers, so no need to call ExecOpenIndices.
	 */

	return rInfo;
}

/*
 * Return the ancestor relations of a given leaf partition result relation
 * up to and including the query's root target relation.
 *
 * These work much like the ones opened by ExecGetTriggerResultRel, except
 * that we need to keep them in a separate list.
 *
 * These are closed by ExecCloseResultRelations.
 */
List *
ExecGetAncestorResultRels(EState *estate, ResultRelInfo *resultRelInfo)
{
	ResultRelInfo *rootRelInfo = resultRelInfo->ri_RootResultRelInfo;
	Relation	partRel = resultRelInfo->ri_RelationDesc;
	Oid			rootRelOid;

	if (!partRel->rd_rel->relispartition)
		elog(ERROR, "cannot find ancestors of a non-partition result relation");
	Assert(rootRelInfo != NULL);
	rootRelOid = RelationGetRelid(rootRelInfo->ri_RelationDesc);
	if (resultRelInfo->ri_ancestorResultRels == NIL)
	{
		ListCell   *lc;
		List	   *oids = get_partition_ancestors(RelationGetRelid(partRel));
		List	   *ancResultRels = NIL;

		foreach(lc, oids)
		{
			Oid			ancOid = lfirst_oid(lc);
			Relation	ancRel;
			ResultRelInfo *rInfo;

			/*
			 * Ignore the root ancestor here, and use ri_RootResultRelInfo
			 * (below) for it instead.  Also, we stop climbing up the
			 * hierarchy when we find the table that was mentioned in the
			 * query.
			 */
			if (ancOid == rootRelOid)
				break;

			/*
			 * All ancestors up to the root target relation must have been
			 * locked by the planner or AcquireExecutorLocks().
			 */
			ancRel = table_open(ancOid, NoLock);
			rInfo = makeNode(ResultRelInfo);

			/* dummy rangetable index */
			InitResultRelInfo(rInfo, ancRel, 0, NULL,
							  estate->es_instrument);
			ancResultRels = lappend(ancResultRels, rInfo);
		}
		ancResultRels = lappend(ancResultRels, rootRelInfo);
		resultRelInfo->ri_ancestorResultRels = ancResultRels;
	}

	/* We must have found some ancestor */
	Assert(resultRelInfo->ri_ancestorResultRels != NIL);

	return resultRelInfo->ri_ancestorResultRels;
}

/* ----------------------------------------------------------------
 *		ExecPostprocessPlan
 *
 *		Give plan nodes a final chance to execute before shutdown
 * ----------------------------------------------------------------
 */
static void
ExecPostprocessPlan(EState *estate)
{
	ListCell   *lc;

	/*
	 * Make sure nodes run forward.
	 */
	estate->es_direction = ForwardScanDirection;

	/*
	 * Run any secondary ModifyTable nodes to completion, in case the main
	 * query did not fetch all rows from them.  (We do this to ensure that
	 * such nodes have predictable results.)
	 */
	foreach(lc, estate->es_auxmodifytables)
	{
		PlanState  *ps = (PlanState *) lfirst(lc);

		for (;;)
		{
			TupleTableSlot *slot;

			/* Reset the per-output-tuple exprcontext each time */
			ResetPerTupleExprContext(estate);

			slot = ExecProcNode(ps);

			if (TupIsNull(slot))
				break;
		}
	}
}

/* ----------------------------------------------------------------
 *		ExecEndPlan
 *
 *		Cleans up the query plan -- closes files and frees up storage
 *
 * NOTE: we are no longer very worried about freeing storage per se
 * in this code; FreeExecutorState should be guaranteed to release all
 * memory that needs to be released.  What we are worried about doing
 * is closing relations and dropping buffer pins.  Thus, for example,
 * tuple tables must be cleared or dropped to ensure pins are released.
 * ----------------------------------------------------------------
 */
static void
ExecEndPlan(PlanState *planstate, EState *estate)
{
	ListCell   *l;

	/*
	 * shut down the node-type-specific query processing
	 */
	ExecEndNode(planstate);

	/*
	 * for subplans too
	 */
	foreach(l, estate->es_subplanstates)
	{
		PlanState  *subplanstate = (PlanState *) lfirst(l);

		ExecEndNode(subplanstate);
	}

	/*
	 * destroy the executor's tuple table.  Actually we only care about
	 * releasing buffer pins and tupdesc refcounts; there's no need to pfree
	 * the TupleTableSlots, since the containing memory context is about to go
	 * away anyway.
	 */
	ExecResetTupleTable(estate->es_tupleTable, false);

	/*
	 * Close any Relations that have been opened for range table entries or
	 * result relations.
	 */
	ExecCloseResultRelations(estate);
	ExecCloseRangeTableRelations(estate);
}

/*
 * Close any relations that have been opened for ResultRelInfos.
 */
void
ExecCloseResultRelations(EState *estate)
{
	ListCell   *l;

	/*
	 * close indexes of result relation(s) if any.  (Rels themselves are
	 * closed in ExecCloseRangeTableRelations())
	 *
	 * In addition, close the stub RTs that may be in each resultrel's
	 * ri_ancestorResultRels.
	 */
	foreach(l, estate->es_opened_result_relations)
	{
		ResultRelInfo *resultRelInfo = lfirst(l);
		ListCell   *lc;

		ExecCloseIndices(resultRelInfo);
		foreach(lc, resultRelInfo->ri_ancestorResultRels)
		{
			ResultRelInfo *rInfo = lfirst(lc);

			/*
			 * Ancestors with RTI > 0 (should only be the root ancestor) are
			 * closed by ExecCloseRangeTableRelations.
			 */
			if (rInfo->ri_RangeTableIndex > 0)
				continue;

			table_close(rInfo->ri_RelationDesc, NoLock);
		}
	}

	/* Close any relations that have been opened by ExecGetTriggerResultRel(). */
	foreach(l, estate->es_trig_target_relations)
	{
		ResultRelInfo *resultRelInfo = (ResultRelInfo *) lfirst(l);

		/*
		 * Assert this is a "dummy" ResultRelInfo, see above.  Otherwise we
		 * might be issuing a duplicate close against a Relation opened by
		 * ExecGetRangeTableRelation.
		 */
		Assert(resultRelInfo->ri_RangeTableIndex == 0);

		/*
		 * Since ExecGetTriggerResultRel doesn't call ExecOpenIndices for
		 * these rels, we needn't call ExecCloseIndices either.
		 */
		Assert(resultRelInfo->ri_NumIndices == 0);

		table_close(resultRelInfo->ri_RelationDesc, NoLock);
	}
}

/*
 * Close all relations opened by ExecGetRangeTableRelation().
 *
 * We do not release any locks we might hold on those rels.
 */
void
ExecCloseRangeTableRelations(EState *estate)
{
	int			i;

	for (i = 0; i < estate->es_range_table_size; i++)
	{
		if (estate->es_relations[i])
			table_close(estate->es_relations[i], NoLock);
	}
}

/* ----------------------------------------------------------------
 *		ExecutePlan
 *
 *		Processes the query plan until we have retrieved 'numberTuples' tuples,
 *		moving in the specified direction.
 *
 *		Runs to completion if numberTuples is 0
 * ----------------------------------------------------------------
 */
static void
ExecutePlan(QueryDesc *queryDesc,
			CmdType operation,
			bool sendTuples,
			uint64 numberTuples,
			ScanDirection direction,
			DestReceiver *dest)
{
	EState	   *estate = queryDesc->estate;
	PlanState  *planstate = queryDesc->planstate;
	bool		use_parallel_mode;
	TupleTableSlot *slot;
	uint64		current_tuple_count;
	bool 		is_complete;
	/*
	 * initialize local variables
	 */
	current_tuple_count = 0;

	/*
	 * Set the direction.
	 */
	estate->es_direction = direction;

	/*
	 * Set up parallel mode if appropriate.
	 *
	 * Parallel mode only supports complete execution of a plan.  If we've
	 * already partially executed it, or if the caller asks us to exit early,
	 * we must force the plan to run without parallelism.
	 */
	if (queryDesc->already_executed || numberTuples != 0)
		use_parallel_mode = false;
	else
		use_parallel_mode = queryDesc->plannedstmt->parallelModeNeeded;
	queryDesc->already_executed = true;

	estate->es_use_parallel_mode = use_parallel_mode;
	if (use_parallel_mode)
		EnterParallelMode();

	/*
	 * Loop until we've processed the proper number of tuples from the plan.
	 */
	for (;;)
	{
		/* Reset the per-output-tuple exprcontext */
		ResetPerTupleExprContext(estate);

		/*
		 * Execute the plan and obtain a tuple
		 */
		slot = ExecProcNode(planstate);

		/*
		 * if the tuple is null, then we assume there is nothing more to
		 * process so we just end the loop...
		 */
		if (TupIsNull(slot))
			break;

		/*
		 * If we have a junk filter, then project a new tuple with the junk
		 * removed.
		 *
		 * Store this new "clean" tuple in the junkfilter's resultSlot.
		 * (Formerly, we stored it back over the "dirty" tuple, which is WRONG
		 * because that tuple slot has the wrong descriptor.)
		 */
		if (estate->es_junkFilter != NULL)
			slot = ExecFilterJunk(estate->es_junkFilter, slot);

		/*
		 * If we are supposed to send the tuple somewhere, do so. (In
		 * practice, this is probably always the case at this point.)
		 */
		if (sendTuples)
		{
			/*
			 * If we are not able to send the tuple, we assume the destination
			 * has closed and no more tuples can be sent. If that's the case,
			 * end the loop.
			 */
			is_complete = dest->receiveSlot(slot, dest);
			if (!is_complete)
				break;

		}
				
		/*
		 * Count tuples processed, if this is a SELECT.  (For other operation
		 * types, the ModifyTable plan node must count the appropriate
		 * events.)
		 */
		if (operation == CMD_SELECT)
			(estate->es_processed)++;

		/*
		 * check our tuple count.. if we've processed the proper number then
		 * quit, else loop again and process more tuples.  Zero numberTuples
		 * means no limit.
		 */
		current_tuple_count++;
		if (numberTuples && numberTuples == current_tuple_count)
			break;
	}

	/*
	 * If we know we won't need to back up, we can release resources at this
	 * point.
	 */
	if (!(estate->es_top_eflags & EXEC_FLAG_BACKWARD))
		ExecShutdownNode(planstate);

	if (use_parallel_mode)
		ExitParallelMode();
}


/*
 * ExecRelCheck --- check that tuple meets constraints for result relation
 *
 * Returns NULL if OK, else name of failed check constraint
 */
static const char *
ExecRelCheck(ResultRelInfo *resultRelInfo,
			 TupleTableSlot *slot, EState *estate)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	int			ncheck = rel->rd_att->constr->num_check;
	ConstrCheck *check = rel->rd_att->constr->check;
	ExprContext *econtext;
	MemoryContext oldContext;
	int			i;

	/*
	 * CheckConstraintFetch let this pass with only a warning, but now we
	 * should fail rather than possibly failing to enforce an important
	 * constraint.
	 */
	if (ncheck != rel->rd_rel->relchecks)
		elog(ERROR, "%d pg_constraint record(s) missing for relation \"%s\"",
			 rel->rd_rel->relchecks - ncheck, RelationGetRelationName(rel));

	/*
	 * If first time through for this result relation, build expression
	 * nodetrees for rel's constraint expressions.  Keep them in the per-query
	 * memory context so they'll survive throughout the query.
	 */
	if (resultRelInfo->ri_ConstraintExprs == NULL)
	{
		oldContext = MemoryContextSwitchTo(estate->es_query_cxt);
		resultRelInfo->ri_ConstraintExprs =
			(ExprState **) palloc0(ncheck * sizeof(ExprState *));
		for (i = 0; i < ncheck; i++)
		{
			Expr	   *checkconstr;

			/* Skip not enforced constraint */
			if (!check[i].ccenforced)
				continue;

			checkconstr = stringToNode(check[i].ccbin);
			checkconstr = (Expr *) expand_generated_columns_in_expr((Node *) checkconstr, rel, 1);
			resultRelInfo->ri_ConstraintExprs[i] =
				ExecPrepareExpr(checkconstr, estate);
		}
		MemoryContextSwitchTo(oldContext);
	}

	/*
	 * We will use the EState's per-tuple context for evaluating constraint
	 * expressions (creating it if it's not already there).
	 */
	econtext = GetPerTupleExprContext(estate);

	/* Arrange for econtext's scan tuple to be the tuple under test */
	econtext->ecxt_scantuple = slot;

	/* And evaluate the constraints */
	for (i = 0; i < ncheck; i++)
	{
		ExprState  *checkconstr = resultRelInfo->ri_ConstraintExprs[i];

		/*
		 * NOTE: SQL specifies that a NULL result from a constraint expression
		 * is not to be treated as a failure.  Therefore, use ExecCheck not
		 * ExecQual.
		 */
		if (checkconstr && !ExecCheck(checkconstr, econtext))
			return check[i].ccname;
	}

	/* NULL result means no error */
	return NULL;
}

/*
 * ExecPartitionCheck --- check that tuple meets the partition constraint.
 *
 * Returns true if it meets the partition constraint.  If the constraint
 * fails and we're asked to emit an error, do so and don't return; otherwise
 * return false.
 */
bool
ExecPartitionCheck(ResultRelInfo *resultRelInfo, TupleTableSlot *slot,
				   EState *estate, bool emitError)
{
	ExprContext *econtext;
	bool		success;

	/*
	 * If first time through, build expression state tree for the partition
	 * check expression.  (In the corner case where the partition check
	 * expression is empty, ie there's a default partition and nothing else,
	 * we'll be fooled into executing this code each time through.  But it's
	 * pretty darn cheap in that case, so we don't worry about it.)
	 */
	if (resultRelInfo->ri_PartitionCheckExpr == NULL)
	{
		/*
		 * Ensure that the qual tree and prepared expression are in the
		 * query-lifespan context.
		 */
		MemoryContext oldcxt = MemoryContextSwitchTo(estate->es_query_cxt);
		List	   *qual = RelationGetPartitionQual(resultRelInfo->ri_RelationDesc);

		resultRelInfo->ri_PartitionCheckExpr = ExecPrepareCheck(qual, estate);
		MemoryContextSwitchTo(oldcxt);
	}

	/*
	 * We will use the EState's per-tuple context for evaluating constraint
	 * expressions (creating it if it's not already there).
	 */
	econtext = GetPerTupleExprContext(estate);

	/* Arrange for econtext's scan tuple to be the tuple under test */
	econtext->ecxt_scantuple = slot;

	/*
	 * As in case of the cataloged constraints, we treat a NULL result as
	 * success here, not a failure.
	 */
	success = ExecCheck(resultRelInfo->ri_PartitionCheckExpr, econtext);

	/* if asked to emit error, don't actually return on failure */
	if (!success && emitError)
		ExecPartitionCheckEmitError(resultRelInfo, slot, estate);

	return success;
}

/*
 * ExecPartitionCheckEmitError - Form and emit an error message after a failed
 * partition constraint check.
 */
void
ExecPartitionCheckEmitError(ResultRelInfo *resultRelInfo,
							TupleTableSlot *slot,
							EState *estate)
{
	Oid			root_relid;
	TupleDesc	tupdesc;
	char	   *val_desc;
	Bitmapset  *modifiedCols;

	/*
	 * If the tuple has been routed, it's been converted to the partition's
	 * rowtype, which might differ from the root table's.  We must convert it
	 * back to the root table's rowtype so that val_desc in the error message
	 * matches the input tuple.
	 */
	if (resultRelInfo->ri_RootResultRelInfo)
	{
		ResultRelInfo *rootrel = resultRelInfo->ri_RootResultRelInfo;
		TupleDesc	old_tupdesc;
		AttrMap    *map;

		root_relid = RelationGetRelid(rootrel->ri_RelationDesc);
		tupdesc = RelationGetDescr(rootrel->ri_RelationDesc);

		old_tupdesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);
		/* a reverse map */
		map = build_attrmap_by_name_if_req(old_tupdesc, tupdesc, false);

		/*
		 * Partition-specific slot's tupdesc can't be changed, so allocate a
		 * new one.
		 */
		if (map != NULL)
			slot = execute_attr_map_slot(map, slot,
										 MakeTupleTableSlot(tupdesc, &TTSOpsVirtual));
		modifiedCols = bms_union(ExecGetInsertedCols(rootrel, estate),
								 ExecGetUpdatedCols(rootrel, estate));
	}
	else
	{
		root_relid = RelationGetRelid(resultRelInfo->ri_RelationDesc);
		tupdesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);
		modifiedCols = bms_union(ExecGetInsertedCols(resultRelInfo, estate),
								 ExecGetUpdatedCols(resultRelInfo, estate));
	}

	val_desc = ExecBuildSlotValueDescription(root_relid,
											 slot,
											 tupdesc,
											 modifiedCols,
											 64);
	ereport(ERROR,
			(errcode(ERRCODE_CHECK_VIOLATION),
			 errmsg("new row for relation \"%s\" violates partition constraint",
					RelationGetRelationName(resultRelInfo->ri_RelationDesc)),
			 val_desc ? errdetail("Failing row contains %s.", val_desc) : 0,
			 errtable(resultRelInfo->ri_RelationDesc)));
}

/*
 * ExecConstraints - check constraints of the tuple in 'slot'
 *
 * This checks the traditional NOT NULL and check constraints.
 *
 * The partition constraint is *NOT* checked.
 *
 * Note: 'slot' contains the tuple to check the constraints of, which may
 * have been converted from the original input tuple after tuple routing.
 * 'resultRelInfo' is the final result relation, after tuple routing.
 */
void
ExecConstraints(ResultRelInfo *resultRelInfo,
				TupleTableSlot *slot, EState *estate)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	TupleDesc	tupdesc = RelationGetDescr(rel);
	TupleConstr *constr = tupdesc->constr;
	Bitmapset  *modifiedCols;

	Assert(constr);				/* we should not be called otherwise */

	if (constr->has_not_null)
	{
		int			natts = tupdesc->natts;
		int			attrChk;

		for (attrChk = 1; attrChk <= natts; attrChk++)
		{
			Form_pg_attribute att = TupleDescAttr(tupdesc, attrChk - 1);

			if (att->attnotnull && slot_attisnull(slot, attrChk))
			{
				char	   *val_desc;
				Relation	orig_rel = rel;
				TupleDesc	orig_tupdesc = RelationGetDescr(rel);

				/*
				 * If the tuple has been routed, it's been converted to the
				 * partition's rowtype, which might differ from the root
				 * table's.  We must convert it back to the root table's
				 * rowtype so that val_desc shown error message matches the
				 * input tuple.
				 */
				if (resultRelInfo->ri_RootResultRelInfo)
				{
					ResultRelInfo *rootrel = resultRelInfo->ri_RootResultRelInfo;
					AttrMap    *map;

					tupdesc = RelationGetDescr(rootrel->ri_RelationDesc);
					/* a reverse map */
					map = build_attrmap_by_name_if_req(orig_tupdesc,
													   tupdesc,
													   false);

					/*
					 * Partition-specific slot's tupdesc can't be changed, so
					 * allocate a new one.
					 */
					if (map != NULL)
						slot = execute_attr_map_slot(map, slot,
													 MakeTupleTableSlot(tupdesc, &TTSOpsVirtual));
					modifiedCols = bms_union(ExecGetInsertedCols(rootrel, estate),
											 ExecGetUpdatedCols(rootrel, estate));
					rel = rootrel->ri_RelationDesc;
				}
				else
					modifiedCols = bms_union(ExecGetInsertedCols(resultRelInfo, estate),
											 ExecGetUpdatedCols(resultRelInfo, estate));
				val_desc = ExecBuildSlotValueDescription(RelationGetRelid(rel),
														 slot,
														 tupdesc,
														 modifiedCols,
														 64);

				ereport(ERROR,
						(errcode(ERRCODE_NOT_NULL_VIOLATION),
						 errmsg("null value in column \"%s\" of relation \"%s\" violates not-null constraint",
								NameStr(att->attname),
								RelationGetRelationName(orig_rel)),
						 val_desc ? errdetail("Failing row contains %s.", val_desc) : 0,
						 errtablecol(orig_rel, attrChk)));
			}
		}
	}

	if (rel->rd_rel->relchecks > 0)
	{
		const char *failed;

		if ((failed = ExecRelCheck(resultRelInfo, slot, estate)) != NULL)
		{
			char	   *val_desc;
			Relation	orig_rel = rel;

			/* See the comment above. */
			if (resultRelInfo->ri_RootResultRelInfo)
			{
				ResultRelInfo *rootrel = resultRelInfo->ri_RootResultRelInfo;
				TupleDesc	old_tupdesc = RelationGetDescr(rel);
				AttrMap    *map;

				tupdesc = RelationGetDescr(rootrel->ri_RelationDesc);
				/* a reverse map */
				map = build_attrmap_by_name_if_req(old_tupdesc,
												   tupdesc,
												   false);

				/*
				 * Partition-specific slot's tupdesc can't be changed, so
				 * allocate a new one.
				 */
				if (map != NULL)
					slot = execute_attr_map_slot(map, slot,
												 MakeTupleTableSlot(tupdesc, &TTSOpsVirtual));
				modifiedCols = bms_union(ExecGetInsertedCols(rootrel, estate),
										 ExecGetUpdatedCols(rootrel, estate));
				rel = rootrel->ri_RelationDesc;
			}
			else
				modifiedCols = bms_union(ExecGetInsertedCols(resultRelInfo, estate),
										 ExecGetUpdatedCols(resultRelInfo, estate));
			val_desc = ExecBuildSlotValueDescription(RelationGetRelid(rel),
													 slot,
													 tupdesc,
													 modifiedCols,
													 64);
			ereport(ERROR,
					(errcode(ERRCODE_CHECK_VIOLATION),
					 errmsg("new row for relation \"%s\" violates check constraint \"%s\"",
							RelationGetRelationName(orig_rel), failed),
					 val_desc ? errdetail("Failing row contains %s.", val_desc) : 0,
					 errtableconstraint(orig_rel, failed)));
		}
	}
}

/*
 * ExecWithCheckOptions -- check that tuple satisfies any WITH CHECK OPTIONs
 * of the specified kind.
 *
 * Note that this needs to be called multiple times to ensure that all kinds of
 * WITH CHECK OPTIONs are handled (both those from views which have the WITH
 * CHECK OPTION set and from row-level security policies).  See ExecInsert()
 * and ExecUpdate().
 */
void
ExecWithCheckOptions(WCOKind kind, ResultRelInfo *resultRelInfo,
					 TupleTableSlot *slot, EState *estate)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	TupleDesc	tupdesc = RelationGetDescr(rel);
	ExprContext *econtext;
	ListCell   *l1,
			   *l2;

	/*
	 * We will use the EState's per-tuple context for evaluating constraint
	 * expressions (creating it if it's not already there).
	 */
	econtext = GetPerTupleExprContext(estate);

	/* Arrange for econtext's scan tuple to be the tuple under test */
	econtext->ecxt_scantuple = slot;

	/* Check each of the constraints */
	forboth(l1, resultRelInfo->ri_WithCheckOptions,
			l2, resultRelInfo->ri_WithCheckOptionExprs)
	{
		WithCheckOption *wco = (WithCheckOption *) lfirst(l1);
		ExprState  *wcoExpr = (ExprState *) lfirst(l2);

		/*
		 * Skip any WCOs which are not the kind we are looking for at this
		 * time.
		 */
		if (wco->kind != kind)
			continue;

		/*
		 * WITH CHECK OPTION checks are intended to ensure that the new tuple
		 * is visible (in the case of a view) or that it passes the
		 * 'with-check' policy (in the case of row security). If the qual
		 * evaluates to NULL or FALSE, then the new tuple won't be included in
		 * the view or doesn't pass the 'with-check' policy for the table.
		 */
		if (!ExecQual(wcoExpr, econtext))
		{
			char	   *val_desc;
			Bitmapset  *modifiedCols;

			switch (wco->kind)
			{
					/*
					 * For WITH CHECK OPTIONs coming from views, we might be
					 * able to provide the details on the row, depending on
					 * the permissions on the relation (that is, if the user
					 * could view it directly anyway).  For RLS violations, we
					 * don't include the data since we don't know if the user
					 * should be able to view the tuple as that depends on the
					 * USING policy.
					 */
				case WCO_VIEW_CHECK:
					/* See the comment in ExecConstraints(). */
					if (resultRelInfo->ri_RootResultRelInfo)
					{
						ResultRelInfo *rootrel = resultRelInfo->ri_RootResultRelInfo;
						TupleDesc	old_tupdesc = RelationGetDescr(rel);
						AttrMap    *map;

						tupdesc = RelationGetDescr(rootrel->ri_RelationDesc);
						/* a reverse map */
						map = build_attrmap_by_name_if_req(old_tupdesc,
														   tupdesc,
														   false);

						/*
						 * Partition-specific slot's tupdesc can't be changed,
						 * so allocate a new one.
						 */
						if (map != NULL)
							slot = execute_attr_map_slot(map, slot,
														 MakeTupleTableSlot(tupdesc, &TTSOpsVirtual));

						modifiedCols = bms_union(ExecGetInsertedCols(rootrel, estate),
												 ExecGetUpdatedCols(rootrel, estate));
						rel = rootrel->ri_RelationDesc;
					}
					else
						modifiedCols = bms_union(ExecGetInsertedCols(resultRelInfo, estate),
												 ExecGetUpdatedCols(resultRelInfo, estate));
					val_desc = ExecBuildSlotValueDescription(RelationGetRelid(rel),
															 slot,
															 tupdesc,
															 modifiedCols,
															 64);

					ereport(ERROR,
							(errcode(ERRCODE_WITH_CHECK_OPTION_VIOLATION),
							 errmsg("new row violates check option for view \"%s\"",
									wco->relname),
							 val_desc ? errdetail("Failing row contains %s.",
												  val_desc) : 0));
					break;
				case WCO_RLS_INSERT_CHECK:
				case WCO_RLS_UPDATE_CHECK:
					if (wco->polname != NULL)
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("new row violates row-level security policy \"%s\" for table \"%s\"",
										wco->polname, wco->relname)));
					else
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("new row violates row-level security policy for table \"%s\"",
										wco->relname)));
					break;
				case WCO_RLS_MERGE_UPDATE_CHECK:
				case WCO_RLS_MERGE_DELETE_CHECK:
					if (wco->polname != NULL)
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("target row violates row-level security policy \"%s\" (USING expression) for table \"%s\"",
										wco->polname, wco->relname)));
					else
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("target row violates row-level security policy (USING expression) for table \"%s\"",
										wco->relname)));
					break;
				case WCO_RLS_CONFLICT_CHECK:
					if (wco->polname != NULL)
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("new row violates row-level security policy \"%s\" (USING expression) for table \"%s\"",
										wco->polname, wco->relname)));
					else
						ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("new row violates row-level security policy (USING expression) for table \"%s\"",
										wco->relname)));
					break;
				default:
					elog(ERROR, "unrecognized WCO kind: %u", wco->kind);
					break;
			}
		}
	}
}

/*
 * ExecBuildSlotValueDescription -- construct a string representing a tuple
 *
 * This is intentionally very similar to BuildIndexValueDescription, but
 * unlike that function, we truncate long field values (to at most maxfieldlen
 * bytes).  That seems necessary here since heap field values could be very
 * long, whereas index entries typically aren't so wide.
 *
 * Also, unlike the case with index entries, we need to be prepared to ignore
 * dropped columns.  We used to use the slot's tuple descriptor to decode the
 * data, but the slot's descriptor doesn't identify dropped columns, so we
 * now need to be passed the relation's descriptor.
 *
 * Note that, like BuildIndexValueDescription, if the user does not have
 * permission to view any of the columns involved, a NULL is returned.  Unlike
 * BuildIndexValueDescription, if the user has access to view a subset of the
 * column involved, that subset will be returned with a key identifying which
 * columns they are.
 */
char *
ExecBuildSlotValueDescription(Oid reloid,
							  TupleTableSlot *slot,
							  TupleDesc tupdesc,
							  Bitmapset *modifiedCols,
							  int maxfieldlen)
{
	StringInfoData buf;
	StringInfoData collist;
	bool		write_comma = false;
	bool		write_comma_collist = false;
	int			i;
	AclResult	aclresult;
	bool		table_perm = false;
	bool		any_perm = false;

	/*
	 * Check if RLS is enabled and should be active for the relation; if so,
	 * then don't return anything.  Otherwise, go through normal permission
	 * checks.
	 */
	if (check_enable_rls(reloid, InvalidOid, true) == RLS_ENABLED)
		return NULL;

	initStringInfo(&buf);

	appendStringInfoChar(&buf, '(');

	/*
	 * Check if the user has permissions to see the row.  Table-level SELECT
	 * allows access to all columns.  If the user does not have table-level
	 * SELECT then we check each column and include those the user has SELECT
	 * rights on.  Additionally, we always include columns the user provided
	 * data for.
	 */
	aclresult = pg_class_aclcheck(reloid, GetUserId(), ACL_SELECT);
	if (aclresult != ACLCHECK_OK)
	{
		/* Set up the buffer for the column list */
		initStringInfo(&collist);
		appendStringInfoChar(&collist, '(');
	}
	else
		table_perm = any_perm = true;

	/* Make sure the tuple is fully deconstructed */
	slot_getallattrs(slot);

	for (i = 0; i < tupdesc->natts; i++)
	{
		bool		column_perm = false;
		char	   *val;
		int			vallen;
		Form_pg_attribute att = TupleDescAttr(tupdesc, i);

		/* ignore dropped columns */
		if (att->attisdropped)
			continue;

		if (!table_perm)
		{
			/*
			 * No table-level SELECT, so need to make sure they either have
			 * SELECT rights on the column or that they have provided the data
			 * for the column.  If not, omit this column from the error
			 * message.
			 */
			aclresult = pg_attribute_aclcheck(reloid, att->attnum,
											  GetUserId(), ACL_SELECT);
			if (bms_is_member(att->attnum - FirstLowInvalidHeapAttributeNumber,
							  modifiedCols) || aclresult == ACLCHECK_OK)
			{
				column_perm = any_perm = true;

				if (write_comma_collist)
					appendStringInfoString(&collist, ", ");
				else
					write_comma_collist = true;

				appendStringInfoString(&collist, NameStr(att->attname));
			}
		}

		if (table_perm || column_perm)
		{
			if (att->attgenerated == ATTRIBUTE_GENERATED_VIRTUAL)
				val = "virtual";
			else if (slot->tts_isnull[i])
				val = "null";
			else
			{
				Oid			foutoid;
				bool		typisvarlena;

				getTypeOutputInfo(att->atttypid,
								  &foutoid, &typisvarlena);
				val = OidOutputFunctionCall(foutoid, slot->tts_values[i]);
			}

			if (write_comma)
				appendStringInfoString(&buf, ", ");
			else
				write_comma = true;

			/* truncate if needed */
			vallen = strlen(val);
			if (vallen <= maxfieldlen)
				appendBinaryStringInfo(&buf, val, vallen);
			else
			{
				vallen = pg_mbcliplen(val, vallen, maxfieldlen);
				appendBinaryStringInfo(&buf, val, vallen);
				appendStringInfoString(&buf, "...");
			}
		}
	}

	/* If we end up with zero columns being returned, then return NULL. */
	if (!any_perm)
		return NULL;

	appendStringInfoChar(&buf, ')');

	if (!table_perm)
	{
		appendStringInfoString(&collist, ") = ");
		appendBinaryStringInfo(&collist, buf.data, buf.len);

		return collist.data;
	}

	return buf.data;
}


/*
 * ExecUpdateLockMode -- find the appropriate UPDATE tuple lock mode for a
 * given ResultRelInfo
 */
LockTupleMode
ExecUpdateLockMode(EState *estate, ResultRelInfo *relinfo)
{
	Bitmapset  *keyCols;
	Bitmapset  *updatedCols;

	/*
	 * Compute lock mode to use.  If columns that are part of the key have not
	 * been modified, then we can use a weaker lock, allowing for better
	 * concurrency.
	 */
	updatedCols = ExecGetAllUpdatedCols(relinfo, estate);
	keyCols = RelationGetIndexAttrBitmap(relinfo->ri_RelationDesc,
										 INDEX_ATTR_BITMAP_KEY);

	if (bms_overlap(keyCols, updatedCols))
		return LockTupleExclusive;

	return LockTupleNoKeyExclusive;
}

/*
 * ExecFindRowMark -- find the ExecRowMark struct for given rangetable index
 *
 * If no such struct, either return NULL or throw error depending on missing_ok
 */
ExecRowMark *
ExecFindRowMark(EState *estate, Index rti, bool missing_ok)
{
	if (rti > 0 && rti <= estate->es_range_table_size &&
		estate->es_rowmarks != NULL)
	{
		ExecRowMark *erm = estate->es_rowmarks[rti - 1];

		if (erm)
			return erm;
	}
	if (!missing_ok)
		elog(ERROR, "failed to find ExecRowMark for rangetable index %u", rti);
	return NULL;
}

/*
 * ExecBuildAuxRowMark -- create an ExecAuxRowMark struct
 *
 * Inputs are the underlying ExecRowMark struct and the targetlist of the
 * input plan node (not planstate node!).  We need the latter to find out
 * the column numbers of the resjunk columns.
 */
ExecAuxRowMark *
ExecBuildAuxRowMark(ExecRowMark *erm, List *targetlist)
{
	ExecAuxRowMark *aerm = (ExecAuxRowMark *) palloc0(sizeof(ExecAuxRowMark));
	char		resname[32];

	aerm->rowmark = erm;

	/* Look up the resjunk columns associated with this rowmark */
	if (erm->markType != ROW_MARK_COPY)
	{
		/* need ctid for all methods other than COPY */
		snprintf(resname, sizeof(resname), "ctid%u", erm->rowmarkId);
		aerm->ctidAttNo = ExecFindJunkAttributeInTlist(targetlist,
													   resname);
		if (!AttributeNumberIsValid(aerm->ctidAttNo))
			elog(ERROR, "could not find junk %s column", resname);
	}
	else
	{
		/* need wholerow if COPY */
		snprintf(resname, sizeof(resname), "wholerow%u", erm->rowmarkId);
		aerm->wholeAttNo = ExecFindJunkAttributeInTlist(targetlist,
														resname);
		if (!AttributeNumberIsValid(aerm->wholeAttNo))
			elog(ERROR, "could not find junk %s column", resname);
	}

	/* if child rel, need tableoid */
	if (erm->rti != erm->prti)
	{
		snprintf(resname, sizeof(resname), "tableoid%u", erm->rowmarkId);
		aerm->toidAttNo = ExecFindJunkAttributeInTlist(targetlist,
													   resname);
		if (!AttributeNumberIsValid(aerm->toidAttNo))
			elog(ERROR, "could not find junk %s column", resname);
	}

	return aerm;
}


/*
 * EvalPlanQual logic --- recheck modified tuple(s) to see if we want to
 * process the updated version under READ COMMITTED rules.
 *
 * See backend/executor/README for some info about how this works.
 */


/*
 * Check the updated version of a tuple to see if we want to process it under
 * READ COMMITTED rules.
 *
 *	epqstate - state for EvalPlanQual rechecking
 *	relation - table containing tuple
 *	rti - rangetable index of table containing tuple
 *	inputslot - tuple for processing - this can be the slot from
 *		EvalPlanQualSlot() for this rel, for increased efficiency.
 *
 * This tests whether the tuple in inputslot still matches the relevant
 * quals. For that result to be useful, typically the input tuple has to be
 * last row version (otherwise the result isn't particularly useful) and
 * locked (otherwise the result might be out of date). That's typically
 * achieved by using table_tuple_lock() with the
 * TUPLE_LOCK_FLAG_FIND_LAST_VERSION flag.
 *
 * Returns a slot containing the new candidate update/delete tuple, or
 * NULL if we determine we shouldn't process the row.
 */
TupleTableSlot *
EvalPlanQual(EPQState *epqstate, Relation relation,
			 Index rti, TupleTableSlot *inputslot)
{
	TupleTableSlot *slot;
	TupleTableSlot *testslot;

	Assert(rti > 0);

	/*
	 * Need to run a recheck subquery.  Initialize or reinitialize EPQ state.
	 */
	EvalPlanQualBegin(epqstate);

	/*
	 * Callers will often use the EvalPlanQualSlot to store the tuple to avoid
	 * an unnecessary copy.
	 */
	testslot = EvalPlanQualSlot(epqstate, relation, rti);
	if (testslot != inputslot)
		ExecCopySlot(testslot, inputslot);

	/*
	 * Mark that an EPQ tuple is available for this relation.  (If there is
	 * more than one result relation, the others remain marked as having no
	 * tuple available.)
	 */
	epqstate->relsubs_done[rti - 1] = false;
	epqstate->relsubs_blocked[rti - 1] = false;

	/*
	 * Run the EPQ query.  We assume it will return at most one tuple.
	 */
	slot = EvalPlanQualNext(epqstate);

	/*
	 * If we got a tuple, force the slot to materialize the tuple so that it
	 * is not dependent on any local state in the EPQ query (in particular,
	 * it's highly likely that the slot contains references to any pass-by-ref
	 * datums that may be present in copyTuple).  As with the next step, this
	 * is to guard against early re-use of the EPQ query.
	 */
	if (!TupIsNull(slot))
		ExecMaterializeSlot(slot);

	/*
	 * Clear out the test tuple, and mark that no tuple is available here.
	 * This is needed in case the EPQ state is re-used to test a tuple for a
	 * different target relation.
	 */
	ExecClearTuple(testslot);
	epqstate->relsubs_blocked[rti - 1] = true;

	return slot;
}

/*
 * EvalPlanQualInit -- initialize during creation of a plan state node
 * that might need to invoke EPQ processing.
 *
 * If the caller intends to use EvalPlanQual(), resultRelations should be
 * a list of RT indexes of potential target relations for EvalPlanQual(),
 * and we will arrange that the other listed relations don't return any
 * tuple during an EvalPlanQual() call.  Otherwise resultRelations
 * should be NIL.
 *
 * Note: subplan/auxrowmarks can be NULL/NIL if they will be set later
 * with EvalPlanQualSetPlan.
 */
void
EvalPlanQualInit(EPQState *epqstate, EState *parentestate,
				 Plan *subplan, List *auxrowmarks,
				 int epqParam, List *resultRelations)
{
	Index		rtsize = parentestate->es_range_table_size;

	/* initialize data not changing over EPQState's lifetime */
	epqstate->parentestate = parentestate;
	epqstate->epqParam = epqParam;
	epqstate->resultRelations = resultRelations;

	/*
	 * Allocate space to reference a slot for each potential rti - do so now
	 * rather than in EvalPlanQualBegin(), as done for other dynamically
	 * allocated resources, so EvalPlanQualSlot() can be used to hold tuples
	 * that *may* need EPQ later, without forcing the overhead of
	 * EvalPlanQualBegin().
	 */
	epqstate->tuple_table = NIL;
	epqstate->relsubs_slot = (TupleTableSlot **)
		palloc0(rtsize * sizeof(TupleTableSlot *));

	/* ... and remember data that EvalPlanQualBegin will need */
	epqstate->plan = subplan;
	epqstate->arowMarks = auxrowmarks;

	/* ... and mark the EPQ state inactive */
	epqstate->origslot = NULL;
	epqstate->recheckestate = NULL;
	epqstate->recheckplanstate = NULL;
	epqstate->relsubs_rowmark = NULL;
	epqstate->relsubs_done = NULL;
	epqstate->relsubs_blocked = NULL;
}

/*
 * EvalPlanQualSetPlan -- set or change subplan of an EPQState.
 *
 * We used to need this so that ModifyTable could deal with multiple subplans.
 * It could now be refactored out of existence.
 */
void
EvalPlanQualSetPlan(EPQState *epqstate, Plan *subplan, List *auxrowmarks)
{
	/* If we have a live EPQ query, shut it down */
	EvalPlanQualEnd(epqstate);
	/* And set/change the plan pointer */
	epqstate->plan = subplan;
	/* The rowmarks depend on the plan, too */
	epqstate->arowMarks = auxrowmarks;
}

/*
 * Return, and create if necessary, a slot for an EPQ test tuple.
 *
 * Note this only requires EvalPlanQualInit() to have been called,
 * EvalPlanQualBegin() is not necessary.
 */
TupleTableSlot *
EvalPlanQualSlot(EPQState *epqstate,
				 Relation relation, Index rti)
{
	TupleTableSlot **slot;

	Assert(relation);
	Assert(rti > 0 && rti <= epqstate->parentestate->es_range_table_size);
	slot = &epqstate->relsubs_slot[rti - 1];

	if (*slot == NULL)
	{
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(epqstate->parentestate->es_query_cxt);
		*slot = table_slot_create(relation, &epqstate->tuple_table);
		MemoryContextSwitchTo(oldcontext);
	}

	return *slot;
}

/*
 * Fetch the current row value for a non-locked relation, identified by rti,
 * that needs to be scanned by an EvalPlanQual operation.  origslot must have
 * been set to contain the current result row (top-level row) that we need to
 * recheck.  Returns true if a substitution tuple was found, false if not.
 */
bool
EvalPlanQualFetchRowMark(EPQState *epqstate, Index rti, TupleTableSlot *slot)
{
	ExecAuxRowMark *earm = epqstate->relsubs_rowmark[rti - 1];
	ExecRowMark *erm;
	Datum		datum;
	bool		isNull;

	Assert(earm != NULL);
	Assert(epqstate->origslot != NULL);

	erm = earm->rowmark;

	if (RowMarkRequiresRowShareLock(erm->markType))
		elog(ERROR, "EvalPlanQual doesn't support locking rowmarks");

	/* if child rel, must check whether it produced this row */
	if (erm->rti != erm->prti)
	{
		Oid			tableoid;

		datum = ExecGetJunkAttribute(epqstate->origslot,
									 earm->toidAttNo,
									 &isNull);
		/* non-locked rels could be on the inside of outer joins */
		if (isNull)
			return false;

		tableoid = DatumGetObjectId(datum);

		Assert(OidIsValid(erm->relid));
		if (tableoid != erm->relid)
		{
			/* this child is inactive right now */
			return false;
		}
	}

	if (erm->markType == ROW_MARK_REFERENCE)
	{
		Assert(erm->relation != NULL);

		/* fetch the tuple's ctid */
		datum = ExecGetJunkAttribute(epqstate->origslot,
									 earm->ctidAttNo,
									 &isNull);
		/* non-locked rels could be on the inside of outer joins */
		if (isNull)
			return false;

		/* fetch requests on foreign tables must be passed to their FDW */
		if (erm->relation->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
		{
			FdwRoutine *fdwroutine;
			bool		updated = false;

			fdwroutine = GetFdwRoutineForRelation(erm->relation, false);
			/* this should have been checked already, but let's be safe */
			if (fdwroutine->RefetchForeignRow == NULL)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("cannot lock rows in foreign table \"%s\"",
								RelationGetRelationName(erm->relation))));

			fdwroutine->RefetchForeignRow(epqstate->recheckestate,
										  erm,
										  datum,
										  slot,
										  &updated);
			if (TupIsNull(slot))
				elog(ERROR, "failed to fetch tuple for EvalPlanQual recheck");

			/*
			 * Ideally we'd insist on updated == false here, but that assumes
			 * that FDWs can track that exactly, which they might not be able
			 * to.  So just ignore the flag.
			 */
			return true;
		}
		else
		{
			/* ordinary table, fetch the tuple */
			if (!table_tuple_fetch_row_version(erm->relation,
											   (ItemPointer) DatumGetPointer(datum),
											   SnapshotAny, slot))
				elog(ERROR, "failed to fetch tuple for EvalPlanQual recheck");
			return true;
		}
	}
	else
	{
		Assert(erm->markType == ROW_MARK_COPY);

		/* fetch the whole-row Var for the relation */
		datum = ExecGetJunkAttribute(epqstate->origslot,
									 earm->wholeAttNo,
									 &isNull);
		/* non-locked rels could be on the inside of outer joins */
		if (isNull)
			return false;

		ExecStoreHeapTupleDatum(datum, slot);
		return true;
	}
}

/*
 * Fetch the next row (if any) from EvalPlanQual testing
 *
 * (In practice, there should never be more than one row...)
 */
TupleTableSlot *
EvalPlanQualNext(EPQState *epqstate)
{
	MemoryContext oldcontext;
	TupleTableSlot *slot;

	oldcontext = MemoryContextSwitchTo(epqstate->recheckestate->es_query_cxt);
	slot = ExecProcNode(epqstate->recheckplanstate);
	MemoryContextSwitchTo(oldcontext);

	return slot;
}

/*
 * Initialize or reset an EvalPlanQual state tree
 */
void
EvalPlanQualBegin(EPQState *epqstate)
{
	EState	   *parentestate = epqstate->parentestate;
	EState	   *recheckestate = epqstate->recheckestate;

	if (recheckestate == NULL)
	{
		/* First time through, so create a child EState */
		EvalPlanQualStart(epqstate, epqstate->plan);
	}
	else
	{
		/*
		 * We already have a suitable child EPQ tree, so just reset it.
		 */
		Index		rtsize = parentestate->es_range_table_size;
		PlanState  *rcplanstate = epqstate->recheckplanstate;

		/*
		 * Reset the relsubs_done[] flags to equal relsubs_blocked[], so that
		 * the EPQ run will never attempt to fetch tuples from blocked target
		 * relations.
		 */
		memcpy(epqstate->relsubs_done, epqstate->relsubs_blocked,
			   rtsize * sizeof(bool));

		/* Recopy current values of parent parameters */
		if (parentestate->es_plannedstmt->paramExecTypes != NIL)
		{
			int			i;

			/*
			 * Force evaluation of any InitPlan outputs that could be needed
			 * by the subplan, just in case they got reset since
			 * EvalPlanQualStart (see comments therein).
			 */
			ExecSetParamPlanMulti(rcplanstate->plan->extParam,
								  GetPerTupleExprContext(parentestate));

			i = list_length(parentestate->es_plannedstmt->paramExecTypes);

			while (--i >= 0)
			{
				/* copy value if any, but not execPlan link */
				recheckestate->es_param_exec_vals[i].value =
					parentestate->es_param_exec_vals[i].value;
				recheckestate->es_param_exec_vals[i].isnull =
					parentestate->es_param_exec_vals[i].isnull;
			}
		}

		/*
		 * Mark child plan tree as needing rescan at all scan nodes.  The
		 * first ExecProcNode will take care of actually doing the rescan.
		 */
		rcplanstate->chgParam = bms_add_member(rcplanstate->chgParam,
											   epqstate->epqParam);
	}
}

/*
 * Start execution of an EvalPlanQual plan tree.
 *
 * This is a cut-down version of ExecutorStart(): we copy some state from
 * the top-level estate rather than initializing it fresh.
 */
static void
EvalPlanQualStart(EPQState *epqstate, Plan *planTree)
{
	EState	   *parentestate = epqstate->parentestate;
	Index		rtsize = parentestate->es_range_table_size;
	EState	   *rcestate;
	MemoryContext oldcontext;
	ListCell   *l;

	epqstate->recheckestate = rcestate = CreateExecutorState();

	oldcontext = MemoryContextSwitchTo(rcestate->es_query_cxt);

	/* signal that this is an EState for executing EPQ */
	rcestate->es_epq_active = epqstate;

	/*
	 * Child EPQ EStates share the parent's copy of unchanging state such as
	 * the snapshot, rangetable, and external Param info.  They need their own
	 * copies of local state, including a tuple table, es_param_exec_vals,
	 * result-rel info, etc.
	 *
	 * es_cachedplan is not copied because EPQ plan execution does not acquire
	 * any new locks that could invalidate the CachedPlan.
	 */
	rcestate->es_direction = ForwardScanDirection;
	rcestate->es_snapshot = parentestate->es_snapshot;
	rcestate->es_crosscheck_snapshot = parentestate->es_crosscheck_snapshot;
	rcestate->es_range_table = parentestate->es_range_table;
	rcestate->es_range_table_size = parentestate->es_range_table_size;
	rcestate->es_relations = parentestate->es_relations;
	rcestate->es_rowmarks = parentestate->es_rowmarks;
	rcestate->es_rteperminfos = parentestate->es_rteperminfos;
	rcestate->es_plannedstmt = parentestate->es_plannedstmt;
	rcestate->es_junkFilter = parentestate->es_junkFilter;
	rcestate->es_output_cid = parentestate->es_output_cid;
	rcestate->es_queryEnv = parentestate->es_queryEnv;

	/*
	 * ResultRelInfos needed by subplans are initialized from scratch when the
	 * subplans themselves are initialized.
	 */
	rcestate->es_result_relations = NULL;
	/* es_trig_target_relations must NOT be copied */
	rcestate->es_top_eflags = parentestate->es_top_eflags;
	rcestate->es_instrument = parentestate->es_instrument;
	/* es_auxmodifytables must NOT be copied */

	/*
	 * The external param list is simply shared from parent.  The internal
	 * param workspace has to be local state, but we copy the initial values
	 * from the parent, so as to have access to any param values that were
	 * already set from other parts of the parent's plan tree.
	 */
	rcestate->es_param_list_info = parentestate->es_param_list_info;
	if (parentestate->es_plannedstmt->paramExecTypes != NIL)
	{
		int			i;

		/*
		 * Force evaluation of any InitPlan outputs that could be needed by
		 * the subplan.  (With more complexity, maybe we could postpone this
		 * till the subplan actually demands them, but it doesn't seem worth
		 * the trouble; this is a corner case already, since usually the
		 * InitPlans would have been evaluated before reaching EvalPlanQual.)
		 *
		 * This will not touch output params of InitPlans that occur somewhere
		 * within the subplan tree, only those that are attached to the
		 * ModifyTable node or above it and are referenced within the subplan.
		 * That's OK though, because the planner would only attach such
		 * InitPlans to a lower-level SubqueryScan node, and EPQ execution
		 * will not descend into a SubqueryScan.
		 *
		 * The EState's per-output-tuple econtext is sufficiently short-lived
		 * for this, since it should get reset before there is any chance of
		 * doing EvalPlanQual again.
		 */
		ExecSetParamPlanMulti(planTree->extParam,
							  GetPerTupleExprContext(parentestate));

		/* now make the internal param workspace ... */
		i = list_length(parentestate->es_plannedstmt->paramExecTypes);
		rcestate->es_param_exec_vals = (ParamExecData *)
			palloc0(i * sizeof(ParamExecData));
		/* ... and copy down all values, whether really needed or not */
		while (--i >= 0)
		{
			/* copy value if any, but not execPlan link */
			rcestate->es_param_exec_vals[i].value =
				parentestate->es_param_exec_vals[i].value;
			rcestate->es_param_exec_vals[i].isnull =
				parentestate->es_param_exec_vals[i].isnull;
		}
	}

	/*
	 * Copy es_unpruned_relids so that pruned relations are ignored by
	 * ExecInitLockRows() and ExecInitModifyTable() when initializing the plan
	 * trees below.
	 */
	rcestate->es_unpruned_relids = parentestate->es_unpruned_relids;

	/*
	 * Initialize private state information for each SubPlan.  We must do this
	 * before running ExecInitNode on the main query tree, since
	 * ExecInitSubPlan expects to be able to find these entries. Some of the
	 * SubPlans might not be used in the part of the plan tree we intend to
	 * run, but since it's not easy to tell which, we just initialize them
	 * all.
	 */
	Assert(rcestate->es_subplanstates == NIL);
	foreach(l, parentestate->es_plannedstmt->subplans)
	{
		Plan	   *subplan = (Plan *) lfirst(l);
		PlanState  *subplanstate;

		subplanstate = ExecInitNode(subplan, rcestate, 0);
		rcestate->es_subplanstates = lappend(rcestate->es_subplanstates,
											 subplanstate);
	}

	/*
	 * Build an RTI indexed array of rowmarks, so that
	 * EvalPlanQualFetchRowMark() can efficiently access the to be fetched
	 * rowmark.
	 */
	epqstate->relsubs_rowmark = (ExecAuxRowMark **)
		palloc0(rtsize * sizeof(ExecAuxRowMark *));
	foreach(l, epqstate->arowMarks)
	{
		ExecAuxRowMark *earm = (ExecAuxRowMark *) lfirst(l);

		epqstate->relsubs_rowmark[earm->rowmark->rti - 1] = earm;
	}

	/*
	 * Initialize per-relation EPQ tuple states.  Result relations, if any,
	 * get marked as blocked; others as not-fetched.
	 */
	epqstate->relsubs_done = palloc_array(bool, rtsize);
	epqstate->relsubs_blocked = palloc0_array(bool, rtsize);

	foreach(l, epqstate->resultRelations)
	{
		int			rtindex = lfirst_int(l);

		Assert(rtindex > 0 && rtindex <= rtsize);
		epqstate->relsubs_blocked[rtindex - 1] = true;
	}

	memcpy(epqstate->relsubs_done, epqstate->relsubs_blocked,
		   rtsize * sizeof(bool));

	/*
	 * Initialize the private state information for all the nodes in the part
	 * of the plan tree we need to run.  This opens files, allocates storage
	 * and leaves us ready to start processing tuples.
	 */
	epqstate->recheckplanstate = ExecInitNode(planTree, rcestate, 0);

	MemoryContextSwitchTo(oldcontext);
}

/*
 * EvalPlanQualEnd -- shut down at termination of parent plan state node,
 * or if we are done with the current EPQ child.
 *
 * This is a cut-down version of ExecutorEnd(); basically we want to do most
 * of the normal cleanup, but *not* close result relations (which we are
 * just sharing from the outer query).  We do, however, have to close any
 * result and trigger target relations that got opened, since those are not
 * shared.  (There probably shouldn't be any of the latter, but just in
 * case...)
 */
void
EvalPlanQualEnd(EPQState *epqstate)
{
	EState	   *estate = epqstate->recheckestate;
	Index		rtsize;
	MemoryContext oldcontext;
	ListCell   *l;

	rtsize = epqstate->parentestate->es_range_table_size;

	/*
	 * We may have a tuple table, even if EPQ wasn't started, because we allow
	 * use of EvalPlanQualSlot() without calling EvalPlanQualBegin().
	 */
	if (epqstate->tuple_table != NIL)
	{
		memset(epqstate->relsubs_slot, 0,
			   rtsize * sizeof(TupleTableSlot *));
		ExecResetTupleTable(epqstate->tuple_table, true);
		epqstate->tuple_table = NIL;
	}

	/* EPQ wasn't started, nothing further to do */
	if (estate == NULL)
		return;

	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	ExecEndNode(epqstate->recheckplanstate);

	foreach(l, estate->es_subplanstates)
	{
		PlanState  *subplanstate = (PlanState *) lfirst(l);

		ExecEndNode(subplanstate);
	}

	/* throw away the per-estate tuple table, some node may have used it */
	ExecResetTupleTable(estate->es_tupleTable, false);

	/* Close any result and trigger target relations attached to this EState */
	ExecCloseResultRelations(estate);

	MemoryContextSwitchTo(oldcontext);

	FreeExecutorState(estate);

	/* Mark EPQState idle */
	epqstate->origslot = NULL;
	epqstate->recheckestate = NULL;
	epqstate->recheckplanstate = NULL;
	epqstate->relsubs_rowmark = NULL;
	epqstate->relsubs_done = NULL;
	epqstate->relsubs_blocked = NULL;
}


bool fairness_check(char **column_headers, CharPtrVector* vec,  SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index)
{
	AttrWithInto *item0 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 0);
	AttrWithInto *item1 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 1);

	int Wr, Wb, epsilon;
	int natts = vec->natts;
	int idx;
	Wr = ((A_Const*)item0->into_val)->val.ival.ival;
	Wb = ((A_Const*)item1->into_val)->val.ival.ival;
	char* Cr = item0->attr_name;
	char* Cb = item1->attr_name;
	char *column_name = subjectToStmt->attr;
	epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
	idx = get_attribute_index(column_headers, natts, column_name);
	int count_feature1=0;
	int count_feature2=0;
	
	for(int i =0;i<vec->size;i++)
	{
		if(atof(vec->data[i][col_index]) > upper_bound+0.00001 || atof(vec->data[i][col_index]) < lower_bound-0.00001)
		{
			continue;
		}
		if(strcmp(vec->data[i][idx], Cr) == 0)
		{
			count_feature1++;
		}
		else if(strcmp(vec->data[i][idx], Cb) == 0)
		{
			count_feature2++;
		}
	}
	int term_to_compare = abs(count_feature1*Wr- count_feature2*Wb);
	if(term_to_compare <= epsilon)
	{
		elog(INFO, "FAIR QUERY OUTPUT");
		return true;
	}
	elog(INFO, "UNFAIR QUERY OUTPUT");
	return false;
}

bool parse_string(const char *input, double *Wr, char *Cr, double *Wb, char *Cb, double *epsilon, char *column_name)
{

	const char *ptr = input;

	const char *start = strstr(ptr, "subject");
	if (!start)
	{
		return false;
	}

	start += strlen("subject");
	while (*start && isspace((unsigned char)*start))
	{
		start++;
	}

	char *tempPtr = column_name;
	while (*start && *start != '|')
	{
		*tempPtr++ = *start++;
	}
	*tempPtr = '\0';
	char subject[150];
	snprintf(subject, sizeof(subject), "subject %s|", column_name);
	char *end;

	end = column_name + strlen(column_name) - 1;
	while (end > column_name && isspace((unsigned char)*end))
		end--;

	*(end + 1) = '\0';
	*Wr = 1.0;
	*Wb = 1.0;
	Cr[0] = '\0';
	Cb[0] = '\0';

	while (*ptr && strncmp(ptr, subject, strlen(subject)) != 0)
	{
		ptr++;
	}
	ptr += strlen(subject);

	while (*ptr && !isdigit(*ptr) && !isalpha(*ptr) && *ptr != '.')
		ptr++;
	if (isdigit(*ptr) || *ptr == '.')
	{
		*Wr = strtof(ptr, (char **)&ptr);
		while (*ptr && !isalpha(*ptr))
			ptr++;
		while (*ptr && isalpha(*ptr))
		{
			*Cr++ = *ptr++;
		}
		*Cr = '\0';
	}
	else if (isalpha(*ptr))
	{
		while (*ptr && isalpha(*ptr))
		{
			*Cr++ = *ptr++;
		}
		*Cr = '\0';
	}
	const char *p = input;
	while (*p && *p != '-')
		p++;
	p++; 
	while (*p && !isdigit(*p) && !isalpha(*p) && *p != '.')
		p++;
	if (isdigit(*p) || *p == '.')
	{
		*Wb = strtof(p, (char **)&p);
		while (*p && !isalpha(*p))
			p++;
		while (*p && isalpha(*p))
		{
			*Cb++ = *p++;
		}
		*Cb = '\0';
	}
	else if (isalpha(*p))
	{
		while (*p && isalpha(*p))
		{
			*Cb++ = *p++;
		}
		*Cb = '\0';
	}
	const char *pt = input;

	while (*pt && strncmp(pt, "subject", strlen("subject")) != 0)
		pt++;

	while (*pt && !(*pt == '<' && *(pt + 1) == '='))
		pt++;
	if (*pt == '<' && *(pt + 1) == '=')
	{
		pt += 2; 
		while (*pt && !isdigit(*pt) && *pt != '.' && *pt != '-')
			pt++;
		*epsilon = strtof(pt, NULL);
	}
	else
	{
		*epsilon = 0.0;
	}
	return true;
}
int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}
int get_attribute_index(char **column_headers, int natts, char *attribute) {
    for (int i = 0; i < natts; i++) {
        if (my_strcmp(column_headers[i], attribute) == 0) {
            return i;
        }
    }
	elog(ERROR, "Attribute not found");
    return -1; // Attribute not found
}

int get_lower_value_index(int index, float value) {
	int nearest_index = -1;
	float max_value = -1;

	for (int i = 0; i < num_tuples; i++) {
		float current_value = atof(outputArray[i][index]);
		if (current_value <= value && (nearest_index == -1 || current_value > max_value)) {
			max_value = current_value;
			nearest_index = i;
		}
	}

	if (nearest_index != -1) {
		return nearest_index;
	}

	elog(ERROR, "No value <= %f found in the relation", value);
	return -1; // No value <= value found
}

int get_upper_value_index(int index, float value) {
	int nearest_index = -1;

	for (int i = 0; i < num_tuples; i++) {
		float current_value = atof(outputArray[i][index]);
		if (current_value >= value && (nearest_index == -1 || current_value < atof(outputArray[nearest_index][index]))) {
			nearest_index = i;
		}
	}

	if (nearest_index != -1) {
		return nearest_index;
	}

	elog(ERROR, "No value >= %f found in the relation", value);
	return -1; // No value >= value found
}

void merge(int left, int mid, int right, int attr_index) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    char ***L = (char ***)malloc(n1 * sizeof(char **));
    char ***R = (char ***)malloc(n2 * sizeof(char **));
    for (int i = 0; i < n1; i++) {
        L[i] = (char **)malloc(MAX_ATTRS * sizeof(char *));
    }
    for (int i = 0; i < n2; i++) {
        R[i] = (char **)malloc(MAX_ATTRS * sizeof(char *));
    }

    for (int i = 0; i < n1; i++)
        for (int j = 0; j < MAX_ATTRS; j++){
            L[i][j] = outputArray[left + i][j];
		}
    for (int i = 0; i < n2; i++)
        for (int j = 0; j < MAX_ATTRS; j++){
            R[i][j] = outputArray[mid + 1 + i][j];
		}

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (atof(L[i][attr_index]) <= atof(R[j][attr_index])) {
            for (int l = 0; l < MAX_ATTRS; l++)
			outputArray[k][l] = L[i][l];
            i++;
        } else {
            for (int l = 0; l < MAX_ATTRS; l++)
			outputArray[k][l] = R[j][l];
            j++;
        }
        k++;
    }
		

    while (i < n1) {
        for (int l = 0; l < MAX_ATTRS; l++)
		outputArray[k][l] = L[i][l];
        i++;
        k++;
    }

    while (j < n2) {
        for (int l = 0; l < MAX_ATTRS; l++)
		outputArray[k][l] = R[j][l];
        j++;
        k++;
    }

    for (int i = 0; i < n1; i++) {
        free(L[i]);
    }
    for (int i = 0; i < n2; i++) {
        free(R[i]);
    }
    free(L);
    free(R);
	
}
void merge_sort(int left, int right, int attr_index) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        merge_sort(left, mid, attr_index);
        merge_sort(mid + 1, right, attr_index);

        merge(left, mid, right, attr_index);
    }
}
typedef struct heapnode
{
	struct heapnode *first_child;
	struct heapnode *next_sibling;
	struct heap_node *prev_or_parent;
	Range r;
	float similarity;
	
} heapnode;
static int int_rbtree_comparator(const RBTNode *a, const RBTNode *b, void *arg)
{
	const IntRBNode *nodeA = (const IntRBNode *) a;
	const IntRBNode *nodeB = (const IntRBNode *) b;

	if (nodeA->key < nodeB->key)
		return -1;
	else if (nodeA->key > nodeB->key)
		return 1;
	return 0;
}
static int heapnode_comparator(const pairingheap_node *a,
							   const pairingheap_node *b,
							   void *arg)
{
	const heapnode *nodeA = (const heapnode *) a;
	const heapnode *nodeB = (const heapnode *) b;

	if (nodeA->similarity > nodeB->similarity)
		return 1; // nodeA has higher similarity
	else if (nodeA->similarity < nodeB->similarity)
		return -1; // nodeB has higher similarity
	else
		return 0; // Both have equal similarity
}
static void int_rbtree_combiner(RBTNode *existing, const RBTNode *newdata, void *arg)
{
    IntRBNode *existNode = (IntRBNode *) existing;
    const IntRBNode *newNode = (const IntRBNode *) newdata;
    /* Overwrite existing value */
   
}

static RBTNode *int_rbtree_allocfunc(void *arg)
{
    IntRBNode *newNode = (IntRBNode *) palloc(sizeof(IntRBNode));
    newNode->key = 0;
	// elog(INFO,"%d",num_tuples * sizeof(int));
	newNode->values = (int *) palloc(num_tuples * sizeof(int));
	if (!newNode->values) {
		perror("Failed to allocate values");
		exit(EXIT_FAILURE);
	}
	newNode->index = 0;
	newNode->start = 0;
    return (RBTNode *) newNode;
}
static void int_rbtree_freefunc(RBTNode *node, void *arg)
{
    pfree(node);
}
void buildingpointers(char **column_headers, char *attribute, int natts, SubjectToStmt* subjectToStmt)
{
	elog(INFO, "Preprocessing the Given Query");
	bool newinsert;
	int cumulative;
	
	int sort_attr_index = get_attribute_index(column_headers, natts, attribute);
    int attr_index = get_attribute_index(column_headers, natts, subjectToStmt->attr);
	// elog(INFO, "Attribute index: %d", attr_index);
	// count_tuples();
	// elog(INFO, "UFFF");
	
    merge_sort(0, num_tuples - 1, sort_attr_index);
	RBTree* tree = rbt_create(sizeof(IntRBNode),   /* Node size */
                          int_rbtree_comparator, /* Comparator */
                          int_rbtree_combiner,   /* Combiner */
                          int_rbtree_allocfunc,  /* Allocator */
                          int_rbtree_freefunc,   /* Free function */
                          NULL); 
    cumulative = 0;
    RJP = (int *)malloc((num_tuples+2) * sizeof(int));
	LJP = (int *)malloc((num_tuples+2) * sizeof(int));
    int *c = (int *)malloc((num_tuples+2) * sizeof(int));
	int *color = (int *)malloc((num_tuples+2) * sizeof(int));
	
    for (int i = 0; i < num_tuples+2; i++) 
	{
        RJP[i] = -1;
		LJP[i] = -1;
    }
	// elog(INFO, "AAAAA");
	AttrWithInto *item0 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 0);
	AttrWithInto *item1 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 1);

	char* feature1 = item0->attr_name;
	char* feature2 = item1->attr_name;
	for (int i = 0; i < num_tuples; i++) {
        if (my_strcmp(outputArray[i][attr_index], feature2) == 0) {
			color[i+1]=-1;
        } else if (my_strcmp(outputArray[i][attr_index], feature1) == 0) {
			color[i+1]=1;
        }
	}
	// elog(INFO,"color[7]:%d",color[7]);
	c[0]=0;
	c[num_tuples+1]=0;
	color[0]=0;
	color[num_tuples+1]=0;
	
    for (int i = 0; i < num_tuples+1; i++) {
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative -= 1;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += 1;
			}
			c[i] = cumulative;
		}	
        
		IntRBNode searchNode;
		searchNode.key = c[i];
		RBTNode* foundNode = rbt_find(tree, (RBTNode *) &searchNode);
		IntRBNode *foundNode1 = (IntRBNode *) foundNode;
		if(foundNode)
		{
			for (int j = foundNode1->start; j < foundNode1->index; j++) 
			{
				RJP[foundNode1->values[j]] = i;

			}
			foundNode1->start = foundNode1->index;
		}
		IntRBNode *newNode = (IntRBNode *) palloc(sizeof(IntRBNode));
		newNode->key = c[i] - color[i+1];
		
		RBTNode* existing = rbt_find(tree, (RBTNode *) newNode);
		if(existing)
		{
			IntRBNode *existNode = (IntRBNode *) existing;
			existNode->values[existNode->index] = i;
			existNode->index++;
		}
		else
		{
			newNode->values = (int *) palloc(num_tuples * sizeof(int));
			newNode->values[0] = i;
			newNode->index = 1;
			newNode->start = 0;
			rbt_insert(tree, (RBTNode *) newNode, &newinsert);
		}

    }
	pfree(tree);
	tree = rbt_create(sizeof(IntRBNode),   /* Node size */
                          int_rbtree_comparator, /* Comparator */
                          int_rbtree_combiner,   /* Combiner */
                          int_rbtree_allocfunc,  /* Allocator */
                          int_rbtree_freefunc,   /* Free function */
                          NULL); 
	c[num_tuples+1]=0;
	cumulative=0;
	// elog(INFO, "Printing right jump pointers:\n");
    // for (int i = 0; i < num_tuples+1; i++) {
    //     elog(INFO, "RJP[%d] = %d\n", i, RJP[i]);
    // }
	for (int i = num_tuples+1; i >=1; i--) {
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative -= 1;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += 1;
			}
			c[i] = cumulative;
		}	
		IntRBNode searchNode;
		searchNode.key = c[i];
		RBTNode* foundNode = rbt_find(tree, (RBTNode *) &searchNode);
		IntRBNode *foundNode1 = (IntRBNode *) foundNode;
		if(foundNode)
		{
			for (int j = foundNode1->start; j < foundNode1->index; j++) 
			{
				LJP[foundNode1->values[j]] = i;
			}
			foundNode1->start = foundNode1->index;
		}
		IntRBNode *newNode = (IntRBNode *) palloc(sizeof(IntRBNode));
		newNode->key = c[i] - color[i-1];
		
		RBTNode* existing = rbt_find(tree, (RBTNode *) newNode);
		if(existing)
		{
			IntRBNode *existNode = (IntRBNode *) existing;
			existNode->values[existNode->index] = i;
			existNode->index++;
		}
		else
		{
			newNode->values = (int *) palloc(num_tuples * sizeof(int));
			newNode->values[0] = i;
			newNode->index = 1;
			newNode->start = 0;
			rbt_insert(tree, (RBTNode *) newNode, &newinsert);
		}

    }

	// Checking if tree iterator works or not
	// RBTreeIterator* iter = (RBTreeIterator *) palloc(sizeof(RBTreeIterator));
	// IntRBNode *node = (IntRBNode *) palloc(sizeof(IntRBNode));
	// node->key = -2;
	// IntRBNode *node1 = (IntRBNode *) rbt_find_great(tree, node, 0);
	// rbt_begin_iterate_from(tree,LeftRightWalk,iter,node1);
	// while(node1){
	// 	elog(INFO, "Key: %d\n", node1->key);
	// 	for (int j = 0; j < node1->index; j++) 
	// 	{
	// 		elog(INFO, "%d\n", node1->values[j]);
	// 	}
	// 	node1 = rbt_iterate(iter);
	// }
	// elog(INFO,"NO ERROR IN JUMP POINTERS");
	// elog(INFO, "Printing left jump pointers:\n");
	// for (int i = 0; i < num_tuples+2; i++) {
	// 	elog(INFO, "LJP[%d] = %d\n", i, LJP[i]);
	// }
	// elog(INFO,"checkin jp function");
	// elog(INFO,"%d",jp(column_headers,sourceText,natts,attribute,13,-1,"left",LJP,RJP,color));
    return;
}

void insert_val(RBTree *rbt, int key, int value){
	IntRBNode node;
	node.key = key;
	IntRBNode *foundNode = (IntRBNode *) rbt_find(rbt, (RBTNode *) &node);
	if(foundNode)
	{
		foundNode->values[foundNode->index] = value;
		foundNode->index++;
	}
	else
	{
		IntRBNode *newNode = (IntRBNode *) palloc(sizeof(IntRBNode));
		newNode->key = key;
		newNode->values = (int *) palloc(num_tuples * sizeof(int));
		newNode->values[0] = value;
		newNode->index = 1;
		newNode->start = 0;
		bool newinsert;
		rbt_insert(rbt, (RBTNode *) newNode, &newinsert);
	}
}
IntRBNode *get_node(RBTree *rbt, int key){
	IntRBNode node;
	node.key = key;
	RBTNode *foundNode = rbt_find(rbt, (RBTNode *) &node);
	return (IntRBNode *) foundNode;
}


void buildingpointers_w(char **column_headers, char *attribute, int natts, SubjectToStmt* subjectToStmt){
	elog(INFO, "Preprocessing buildpointer with weights for the Given Query");
	bool newinsert;
	int cumulative;
	
	int sort_attr_index = get_attribute_index(column_headers, natts, attribute);
	int attr_index = get_attribute_index(column_headers, natts, subjectToStmt->attr);
	merge_sort(0, num_tuples - 1, sort_attr_index);
	RBTree* fwdPosBST = rbt_create(sizeof(IntRBNode),   /* Node size */
						  int_rbtree_comparator, /* Comparator */
						  int_rbtree_combiner,   /* Combiner */
						  int_rbtree_allocfunc,  /* Allocator */
						  int_rbtree_freefunc,   /* Free function */
						  NULL); 
	RBTree* fwdNegBST = rbt_create(sizeof(IntRBNode),   /* Node size */
	int_rbtree_comparator, /* Comparator */
	int_rbtree_combiner,   /* Combiner */
	int_rbtree_allocfunc,  /* Allocator */
	int_rbtree_freefunc,   /* Free function */
	NULL); 
	cumulative = 0;

	fwdPosPtr = (int *)malloc((num_tuples+2) * sizeof(int));
	fwdNegPtr = (int *)malloc((num_tuples+2) * sizeof(int));
	int *c = (int *)malloc((num_tuples+2) * sizeof(int));
	int *color = (int *)malloc((num_tuples+2) * sizeof(int));
	for (int i = 0; i < num_tuples+2; i++) 
	{
		fwdPosPtr[i] = -1;
		fwdNegPtr[i] = -1;
	}
	AttrWithInto *item0 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 0);
	AttrWithInto *item1 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 1);
	int w1;
	if (item0->into_val == NULL) {
		w1 = 1;
	} else {
		w1 = ((A_Const*)item0->into_val)->val.ival.ival;
	}

	int w2;
	if (item1->into_val == NULL) {
		w2 = -1;
	} else {
		w2 = -((A_Const*)item1->into_val)->val.ival.ival;
	}

	char* feature1 = item0->attr_name;
	char* feature2 = item1->attr_name;
	for (int i = 0; i < num_tuples; i++) {
        if (my_strcmp(outputArray[i][attr_index], feature2) == 0) {
			color[i+1]=w2;
        } else if (my_strcmp(outputArray[i][attr_index], feature1) == 0) {
			color[i+1]=w1;
        }
	}
	// elog(INFO,"color[7]:%d",color[7]);
	c[0]=0;
	c[num_tuples+1]=0;
	color[0]=0;
	color[num_tuples+1]=0;
	
	for (int i = 0; i < num_tuples+1; i++){
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative += w2;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += w1;
			}
			c[i] = cumulative;
		}	

		RBTreeIterator *iter = (RBTreeIterator *) palloc(sizeof(RBTreeIterator));
		rbt_begin_iterate(fwdPosBST, LeftRightWalk, iter);
		IntRBNode *node = (IntRBNode *) rbt_iterate(iter);
		while (node && node->key < c[i]) {
			for (int j = node->start; j < node->index; j++) {
				fwdPosPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = rbt_iterate(iter);
		}
		node = (IntRBNode *) palloc(sizeof(IntRBNode));
		node->key = c[i];
		node = rbt_find_great(fwdNegBST, node, 0);
		rbt_begin_iterate_from(fwdNegBST, LeftRightWalk, iter, node);
		while (node) {
			for (int j = node->start; j < node->index; j++) {
				fwdNegPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = rbt_iterate(iter);
		}
		insert_val(fwdPosBST, c[i], i);
		insert_val(fwdNegBST, c[i], i);

	}

	pfree(fwdPosBST);
	pfree(fwdNegBST);

	RBTree* prevPosBST = rbt_create(sizeof(IntRBNode),   /* Node size */
						int_rbtree_comparator, /* Comparator */
						int_rbtree_combiner,   /* Combiner */
						int_rbtree_allocfunc,  /* Allocator */
						int_rbtree_freefunc,   /* Free function */
						NULL); 
	RBTree* prevNegBST = rbt_create(sizeof(IntRBNode),   /* Node size */
	int_rbtree_comparator, /* Comparator */
	int_rbtree_combiner,   /* Combiner */
	int_rbtree_allocfunc,  /* Allocator */
	int_rbtree_freefunc,   /* Free function */
	NULL); 

	cumulative = 0;
	c = (int *)malloc((num_tuples+2) * sizeof(int));
	c[0]=0;
	c[num_tuples+1]=0;
	prevPosPtr = (int *)malloc((num_tuples+2) * sizeof(int));
	prevNegPtr = (int *)malloc((num_tuples+2) * sizeof(int));
	for (int i = 0; i < num_tuples+2; i++) 
	{
		prevPosPtr[i] = -1;
		prevNegPtr[i] = -1;
	}

	for(int i = num_tuples+1; i >=1; i--) {
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative += w2;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += w1;
			}
			c[i] = cumulative;
		}	
		RBTreeIterator *iter = (RBTreeIterator *) palloc(sizeof(RBTreeIterator));
		rbt_begin_iterate(prevPosBST, LeftRightWalk, iter);
		IntRBNode *node = (IntRBNode *) rbt_iterate(iter);
		while (node && node->key < c[i]) {
			for (int j = node->start; j < node->index; j++) {
				prevPosPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = rbt_iterate(iter);
		}
		node = (IntRBNode *) palloc(sizeof(IntRBNode));
		node->key = c[i];
		node = rbt_find_great(prevNegBST, node, 0);
		rbt_begin_iterate_from(prevNegBST, LeftRightWalk, iter, node);
		while (node) {
			for (int j = node->start; j < node->index; j++) {
				prevNegPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = rbt_iterate(iter);
		}
		
		insert_val(prevPosBST, c[i], i);
		insert_val(prevNegBST, c[i], i);
	}

	pfree(prevPosBST);
	pfree(prevNegBST);

	// for(int i = 0; i < num_tuples+2; i++) 
	// {
	// 	elog(INFO, "fwdPosPtr[%d] = %d, fwdNegPtr[%d] = %d, prevPosPtr[%d] = %d, prevNegPtr[%d] = %d\n", i, fwdPosPtr[i], i, fwdNegPtr[i], i, prevPosPtr[i], i, prevNegPtr[i]);
	// }
}

int  jp(char **column_headers, char *sourceText, int natts, char *attribute,int curr,int color,char * dir,int *LJP,int* RJP,int * colorarray)
{
	if(strcmp(dir,"left")==0)
	{
		if(colorarray[curr-1]==color)
		{
			return curr-1;
		}
		else
		{
			return LJP[curr];
		}
	}
	else 
	{
		if(colorarray[curr+1]==color)
		{
			return curr+1;
		}
		else
		{
			return RJP[curr];
		}
	}
}


void initializestack(mystack* stack, char* name)
{
	stack->stackname = name;
	dlist_init(&stack->header);
}

void pushstack(mystack* stack, int value)
{
	mynode* new_node = (mynode*)palloc(sizeof(mynode));
	new_node->value = value;
	dlist_push_head(&stack->header, &new_node->node);
}

void pop(mystack* stack)
{
	if (dlist_is_empty(&stack->header))
	{
		elog(INFO, "Stack is empty");
		return;
	}
	#define dlist_container(type, membername, ptr) \
		((type *) ((char *) (ptr) - offsetof(type, membername)))

	mynode* top_node = dlist_container(mynode, node, dlist_pop_head_node(&stack->header));
	pfree(top_node);
}

int top(mystack* stack)
{
	if (dlist_is_empty(&stack->header))
	{
		elog(INFO, "Stack is empty");
		return -1;
	}
	#define dlist_container(type, membername, ptr) \
		((type *) ((char *) (ptr) - offsetof(type, membername)))

	mynode* top_node = dlist_container(mynode, node, dlist_head_node(&stack->header));
	return top_node->value;
}

int size(mystack* stack)
{
	int count = 0;
	dlist_iter iter;
	dlist_foreach(iter, &stack->header)
	{
		count++;
	}
	return count;
}

void update_range(Range* range, int actualStart, int actualEnd, int outputStart, int outputEnd)
{
	Range r1 = createRange(actualStart, actualEnd);
	Range r2 = createRange(outputStart, outputEnd);
	double similarity = jaccordsimilarity(r1, r2);
	// elog(INFO, "Update start %d, end %d, similarity %f", outputStart, outputEnd, similarity);
	if(!range->valid || similarity > range->similarity)
	{
		range->start = outputStart;
		range->end = outputEnd;
		range->valid = true;
		range->similarity = similarity;
	}
}

Range getrange(char **column_headers, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon)
{

	if(start <=0 || end > num_tuples)
	{
		elog(ERROR, "Invalid range");
		return createRange(-1,-1);
	}
    int attr_index = get_attribute_index(column_headers, natts, subjectToStmt->attr);

	int *c = (int *)malloc((num_tuples+2) * sizeof(int));
	int *color= (int *)malloc((num_tuples+2) * sizeof(int));
	for (int i = 0; i < num_tuples+2; i++) 
	{
		c[i] = -1;
		color[i] = 0;
	}

	c[0]=0;
	c[num_tuples+1]=0;
	int cumulative = 0;

	AttrWithInto *item0 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 0);
	AttrWithInto *item1 = (AttrWithInto *) list_nth(subjectToStmt->attr_list, 1);
	int w1;
	if (item0->into_val == NULL) {
		w1 = 1;
	} else {
		w1 = ((A_Const*)item0->into_val)->val.ival.ival;
	}

	int w2;
	if (item1->into_val == NULL) {
		w2 = -1;
	} else {
		w2 = -((A_Const*)item1->into_val)->val.ival.ival;
	}

	char* feature1 = item0->attr_name;
	char* feature2 = item1->attr_name;

    for (int i = 0; i < num_tuples+1; i++) {
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += w1;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative += w2;
			}
			c[i] = cumulative;
		}	
	}
	for (int i = 0; i < num_tuples; i++) {
        if (my_strcmp(outputArray[i][attr_index], feature2) == 0) {
			color[i+1]=-1;
        } else if (my_strcmp(outputArray[i][attr_index], feature1) == 0) {
			color[i+1]=1;
        }
	}

	color[0]=-1;
	color[num_tuples+1]=-1;
	int disparity = c[end]- c[start-1];
	
	// elog(INFO, "Printing LJP and RJP arrays:");
	// for (int i = 0; i < num_tuples + 2; i++) {
	// 	elog(INFO, "Colour: %d, LJP[%d] = %d, RJP[%d] = %d, cumulative: %d", color[i], i, LJP[i], i, RJP[i], c[i]);
	// }


	if(abs(disparity)<=epsilon)
	{
		elog(INFO,"Disparity is within the epsilon range");
		return createRange(start,end);
	}
	Range result = createRange(0,0);
	result.valid = false;
	result.similarity = 0;

	int disparityRight = disparity;
	int left = start, right = end;
	mystack rightExpansion;
	initializestack(&rightExpansion, "rightExpansion");
	pushstack(&rightExpansion, right);
	while(abs(disparityRight) > epsilon)
	{
		int stackEnd = top(&rightExpansion);
		int rightEnd;
		if(disparityRight < 0) rightEnd = fwdPosPtr[stackEnd];
		else rightEnd = fwdNegPtr[stackEnd];
		if(rightEnd == -1) break;
		disparityRight = c[rightEnd] - c[left-1];
		pushstack(&rightExpansion, rightEnd);
	}
	if(abs(disparityRight) <= epsilon) update_range(&result, left, right, left, top(&rightExpansion));

	mystack leftExpansion, leftShrink;
	initializestack(&leftExpansion, "leftExpansion");
	initializestack(&leftShrink, "leftShrink");
	pushstack(&leftExpansion, left);
	pushstack(&leftShrink, left);
	bool leftExpansionFail = false, leftShrinkFail = false;
	while(size(&rightExpansion) > 0)
	{
		int newDisparity = c[top(&rightExpansion)] - c[top(&leftExpansion)-1];
		while(abs(newDisparity) > epsilon)
		{
			int newLeftEnd;
			if(newDisparity > 0) newLeftEnd = prevNegPtr[top(&leftExpansion)];
			else newLeftEnd = prevPosPtr[top(&leftExpansion)];
			if(newLeftEnd == -1) {
				leftExpansionFail = true;
				break;
			}
			pushstack(&leftExpansion, newLeftEnd);
			newDisparity = c[top(&rightExpansion)] - c[top(&leftExpansion)-1];
		}
		if(size(&leftExpansion) > 1 && !leftExpansionFail) update_range(&result, left, right, top(&leftExpansion), top(&rightExpansion));

		newDisparity = c[top(&rightExpansion)] - c[top(&leftShrink)-1];

		while(abs(newDisparity) > epsilon)
		{
			int newLeftEnd;
			if(newDisparity > 0) newLeftEnd = fwdPosPtr[top(&leftShrink) - 1];
			else newLeftEnd = fwdNegPtr[top(&leftShrink) - 1];
			if(newLeftEnd == -1) {
				leftShrinkFail = true;
				break;
			}
			pushstack(&leftShrink, newLeftEnd + 1);
			newDisparity = c[top(&rightExpansion)] - c[top(&leftShrink)-1];
		}

		if(size(&leftShrink) > 1 && !leftShrinkFail) update_range(&result, left, right, top(&leftShrink), top(&rightExpansion));

		pop(&rightExpansion);
	}

	mystack rightShrink;
	initializestack(&rightShrink, "rightShrink");
	pushstack(&rightShrink, right);
	disparityRight = c[top(&rightShrink)] - c[left-1];
	while(abs(disparityRight) > epsilon)
	{
		if(abs(c[top(&rightShrink)] - c[top(&leftShrink)-1]) <= epsilon){
			update_range(&result, left, right, top(&leftShrink), top(&rightShrink));
			pop(&leftShrink);
		}

		if(abs(c[top(&rightShrink)] - c[top(&leftExpansion)-1]) <= epsilon){
			update_range(&result, left, right, top(&leftExpansion), top(&rightShrink));
			pop(&leftExpansion);
		}

		int newRightEnd;
		if(disparityRight > 0) newRightEnd = prevPosPtr[top(&rightShrink) + 1] - 1;
		else newRightEnd = prevNegPtr[top(&rightShrink) + 1] - 1;

		if(newRightEnd == -1) break;
		pushstack(&rightShrink, newRightEnd);
		disparityRight = c[top(&rightShrink)] - c[left-1];
	}
	elog(INFO, "result start: %d, end: %d, similarity = %f", result.start, result.end, result.similarity);
	return result;
}


Range multicolor_getrange(CharPtrVector* vec, char **column_headers, char* attribute, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon)
{	
	int num_color = list_length(subjectToStmt->attr_list);

	int *weights = malloc(num_color * sizeof(int));
	char **C = malloc(num_color * sizeof(char *));

	int idx = 0;

	ListCell *lc;
	foreach(lc, subjectToStmt->attr_list)  // stmt is your SubjectToStmt*
	{
		
		AttrWithInto *item = (AttrWithInto *) lfirst(lc);

		weights[idx] = ((A_Const*)item->into_val)->val.ival.ival;
		C[idx] = pstrdup(item->attr_name);

		idx++;
	}
	
	int **prefix_sums;
	double best_similarity = 0;
	Range fair_range = createRange(-1,-1);

	if(start <= 0 || end > num_tuples)
	{
		elog(ERROR, "Invalid range");
		return createRange(-1,-1);
	}

    int attr_index = get_attribute_index(column_headers, natts, subjectToStmt->attr);

	prefix_sums = malloc(num_color * sizeof(int *));

	for (int i = 0; i < num_color; i++)	{
		prefix_sums[i] = malloc((num_tuples) * sizeof(int)); 
	}

	for (int i = 0; i < num_color; i++) {
		for (int j = 0; j < num_tuples; j++) {
			if(j != 0)
			{	if (strcmp(outputArray[j][attr_index], C[i]) == 0) {
					prefix_sums[i][j] = 1 + prefix_sums[i][j - 1];
				} else {
					prefix_sums[i][j] = prefix_sums[i][j - 1];
				}
			}
			else{
				if (strcmp(outputArray[j][attr_index], C[i]) == 0) {
					prefix_sums[i][j] = 1;
				} else {
					prefix_sums[i][j] = 0;
				}
			}
		}
	}


	bool is_fair = true;

	for(int i = 0; i < num_color; i++){
		for (int j = 0; j < num_color; j++){
			if(i != j)
			{
				int count1 = prefix_sums[i][end - 1] - (start == 1 ? 0 : prefix_sums[i][start - 1 - 1]);
				int count2 = prefix_sums[j][end - 1] - (start == 1 ? 0 : prefix_sums[j][start - 1 - 1]);
				int term_to_compare = abs(count1 * weights[i] - count2 * weights[j]);
				if(term_to_compare > epsilon)
				{
					is_fair = false;
				}
			}
		}
	}

	if (is_fair)
	{
		elog(INFO,"FAIR QUERY OUTPUT");
		return createRange(start,end);
	}
	else
	{
		elog(INFO,"UNFAIR QUERY OUTPUT");
	}
	
	for(int i = 0; i < num_tuples; i++){
		for (int j = 0; j < num_tuples; j++)
		{
			bool is_fair_range = true;
			float similarity;
			for (int k = 0; k < num_color; k++)
			{
				for (int l = 0; l < num_color; l++)
				{
					if(k != l)
					{
						int count1 = prefix_sums[k][j] - (i == 0 ? 0 : prefix_sums[k][i - 1]);
						int count2 = prefix_sums[l][j] - (i == 0 ? 0 : prefix_sums[l][i - 1]);
						int term_to_compare = abs(count1 * weights[k] - count2 * weights[l]);
						if(term_to_compare > epsilon)
						{
							is_fair_range = false;
						}
					}
				}
			}

			if (is_fair_range)
			{
				similarity = jaccordsimilarity(createRange(i+1,j+1), createRange(start,end));
				if (similarity > best_similarity)
				{
					best_similarity = similarity;
					fair_range = createRange(i+1,j+1);
				}
			}
		}
	}

	elog(INFO,"Best similarity: %.2f",best_similarity);

	for (int i = 0; i < num_color; i++) {
		free(prefix_sums[i]);
	}
	
	return fair_range;
}


#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
double jaccordsimilarity(Range r1, Range r2){
    int union_r1_r2 = max(r1.end, r2.end) - min(r1.start, r2.start)  + 1;
    int int_r1_r2 = min(r1.end, r2.end) - max(r1.start, r2.start) + 1;
	double sim = (double)int_r1_r2 / (double)union_r1_r2;
    return sim;
}
char *print_table_names_from_query(QueryDesc *queryDesc)
{
    ListCell *lc;

    foreach(lc, queryDesc->plannedstmt->rtable)
    {
        RangeTblEntry *rte = (RangeTblEntry *) lfirst(lc);

        if (rte->rtekind == RTE_RELATION)
        {
            Oid relid = rte->relid;
            const char *relname = get_rel_name(relid);

            if (relname)
                return relname;
            else{
                elog(INFO, "Could not resolve table name for relid %u", relid);
				return NULL;
			}
        }
    }
}


static bool
is_scan_node(Node *node)
{
    NodeTag tag = nodeTag(node);
    return tag == T_SeqScanState ||
           tag == T_IndexScanState ||
           tag == T_BitmapHeapScanState ||
           tag == T_SampleScanState ||
           tag == T_TidScanState ||
           tag == T_SubqueryScanState ||
           tag == T_FunctionScanState ||
           tag == T_ValuesScanState ||
           tag == T_TableFuncScanState ||
           tag == T_CteScanState ||
           tag == T_NamedTuplestoreScanState ||
           tag == T_WorkTableScanState ||
           tag == T_ForeignScanState ||
           tag == T_CustomScanState;
}

void store_table_data(Oid relid, CharPtrVector *vec)
{
    Relation rel;
    TableScanDesc scan;
    HeapTuple tuple;
    TupleDesc tupdesc;

    rel = table_open(relid, AccessShareLock);
    tupdesc = RelationGetDescr(rel);
    vec->natts = tupdesc->natts;

    scan = table_beginscan(rel, GetActiveSnapshot(), 0, NULL);

    while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
    {
        char **row = (char **) palloc(sizeof(char *) * tupdesc->natts);
        bool isnull;

        for (int i = 0; i < tupdesc->natts; i++)
        {
            if (TupleDescAttr(tupdesc, i)->attisdropped)
            {
                row[i] = NULL;
                continue;
            }

            Datum val = heap_getattr(tuple, i + 1, tupdesc, &isnull);
            if (isnull)
            {
                row[i] = NULL;
            }
            else
            {
                Oid typoutput;
                bool typIsVarlena;
                char *str;

                getTypeOutputInfo(TupleDescAttr(tupdesc, i)->atttypid, &typoutput, &typIsVarlena);
                str = OidOutputFunctionCall(typoutput, val);
                row[i] = pstrdup(str); // Make sure it lives in PostgreSQL memory context
            }
        }

        char_ptr_vector_push(vec, row);
    }

    table_endscan(scan);
    table_close(rel, AccessShareLock);
}

bool validrange(Range r)
{
	if(r.start < 0 || r.end < 0 || r.start > num_tuples || r.end > num_tuples)
	{
		return false;
	}
	else if(r.start > r.end)
	{
		return false;
	}
	else
	{
		return true;
	}
}

Range recursivedfs(Range originalrange,char **column_headers, CharPtrVector* vec, SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index)
{
	pairingheap *currheap = pairingheap_allocate(heapnode_comparator, NULL);
	heapnode *first_node = (heapnode *) palloc(sizeof(heapnode));
	first_node->r = originalrange;
	first_node->similarity = jaccordsimilarity(originalrange, originalrange);
	pairingheap_add(currheap, (pairingheap_node *) first_node);	
	
	while(true)
	{
		if(pairingheap_is_empty(currheap))
		{
			break;
		}
		heapnode *maxsimilarity_node = (heapnode *) pairingheap_remove_first(currheap);
		bool topsimilarityfair = fairness_check(column_headers, vec, subjectToStmt, maxsimilarity_node->r.start, maxsimilarity_node->r.end,col_index);
		if(topsimilarityfair)
		{
			return maxsimilarity_node->r;
		}
		int topstart = maxsimilarity_node->r.start;
		int topend = maxsimilarity_node->r.end;
		int topstartminus = topstart - 1;
		int topendplus = topend + 1;
		int topstartplus = topstart + 1;
		int topendminus = topend - 1;
		Range newrange1 = createRange(topstartminus, topendminus);
		Range newrange2 = createRange(topstartminus, topendplus);
		Range newrange3 = createRange(topstartplus, topendminus);
		Range newrange4 = createRange(topstartplus, topendplus);
		if(validrange(newrange1))
		{
			heapnode *new_node1 = (heapnode *) palloc(sizeof(heapnode));
			new_node1->r = newrange1;
			new_node1->similarity = jaccordsimilarity(originalrange, newrange1);
			if(new_node1->similarity < maxsimilarity_node->similarity)(currheap, (pairingheap_node *) new_node1);
		}
		if(validrange(newrange2))
		{
			heapnode *new_node2 = (heapnode *) palloc(sizeof(heapnode));
			new_node2->r = newrange2;
			new_node2->similarity = jaccordsimilarity(originalrange, newrange2);
			if(new_node2->similarity < maxsimilarity_node->similarity)pairingheap_add(currheap, (pairingheap_node *) new_node2);
		}
		if(validrange(newrange3))
		{
			heapnode *new_node3 = (heapnode *) palloc(sizeof(heapnode));
			new_node3->r = newrange3;
			new_node3->similarity = jaccordsimilarity(originalrange, newrange3);
			if(new_node3->similarity < maxsimilarity_node->similarity)pairingheap_add(currheap, (pairingheap_node *) new_node3);
		}
		if(validrange(newrange4))
		{
			heapnode *new_node4 = (heapnode *) palloc(sizeof(heapnode));
			new_node4->r = newrange4;
			new_node4->similarity = jaccordsimilarity(originalrange, newrange4);
			if(new_node4->similarity < maxsimilarity_node->similarity)pairingheap_add(currheap, (pairingheap_node *) new_node4);
		}
	}
	return createRange(-1,-1);
}