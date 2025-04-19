#include "executor/fairness.h"

typedef struct Range {
	int start;
	int end;
	bool valid;
	double similarity;
} Range;

/* Function prototype for createRange */
Range createRange(int st, int e);
bool parse_string(const char *input, double *Wr, char *Cr, double *Wb, char *Cb, double *epsilon, char *column_name);
bool fairness_check_naive(char **column_headers, CharPtrVector* vec, SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index);
bool fairness_check(SubjectToStmt* subjectToStmt, StringVector* color_vector, int **prefixsum, int start, int end);
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
char *print_table_names_from_query(QueryDesc *queryDesc);
Range getrange_LJP_RJP(char **column_headers, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon);
Range getrange(char **column_headers, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon);
Range multicolor_getrange(CharPtrVector* vec, char **column_headers, char* attribute, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon);
Range recursivebfs(Range originalrange, StringVector* color_vector, int **prefix_sum, SubjectToStmt* subjectToStmt);
int int_rbtree_comparator(const RBTNode *a, const RBTNode *b, void *arg);
int weighted_pointers_rbtree_comparator(const RBTNode *a, const RBTNode *b, void *arg);
void int_rbtree_combiner(RBTNode *existing, const RBTNode *newdata, void *arg);
void weighted_pointers_rbtree_combiner(RBTNode *existing, const RBTNode *newdata, void *arg);
RBTNode *int_rbtree_allocfunc(void *arg);
RBTNode *weighted_pointers_rbtree_allocfunc(void *arg);
void int_rbtree_freefunc(RBTNode *node, void *arg);
void weighted_pointers_rbtree_freefunc(RBTNode *node, void *arg);
int **compute_prefix_sums(int num_tuples, char **distinct_colors, int color_count, int color_index);
char **compute_distinct_colors(int num_tuples, int color_index, int *color_count);
int get_lower_value_index(int index, float value);
int get_upper_value_index(int index, float value);
void store_table_data(Oid relid, CharPtrVector *vec);
void pop(mystack* stack);
int top(mystack* stack);
int size(mystack* stack);
bool is_scan_node(Node *node);
bool is_binary_attr(CharPtrVector* vec, int index);

static int *RJP, *LJP;
static WeightedPointersRBNode *weighted_pointers_node;
static int *fwdPosPtr, *fwdNegPtr, *prevPosPtr, *prevNegPtr;
static char ***outputArray;
static BoolVector *reset_outputArrays_vec;
static BoolVector *built_pointers_vec;
static PtrVector *table_pointers;
static PtrVector *table_attributes;
static PtrVector *table_LJP;
static PtrVector *table_RJP;
static PtrVector *table_weighted_pointers;
static PtrVector *table_prefix_sums;
static PtrVector *table_distinct_colors;
static StringVector *tables;
// PtrVector *pointers;
static MemoryContext weightedPointersContext = NULL;
static bool reset_table = true;
static int num_tuples = 0;
static int MAX_ATTRS = 0;
static int sort_attr_index = 0;
static int color_index = 0;


void process_query(QueryDesc* queryDesc, StringVector* column_header, SubjectToStmt* subjectToStmt){
    int epsilon;
    if(reset_table){
		reset_table = false;
		tables = (StringVector *) malloc(sizeof(StringVector));
		string_vector_init(tables);
		table_pointers = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_pointers);
		table_distinct_colors = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_distinct_colors);
		table_prefix_sums = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_prefix_sums);
		table_attributes = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_attributes);
		reset_outputArrays_vec = (BoolVector *) malloc(sizeof(BoolVector));
		bool_vector_init(reset_outputArrays_vec);
		built_pointers_vec = (BoolVector *) malloc(sizeof(BoolVector));
		bool_vector_init(built_pointers_vec);
		table_weighted_pointers = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_weighted_pointers);
		table_LJP = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_LJP);
		table_RJP = (PtrVector *) malloc(sizeof(PtrVector));
		ptr_vector_init(table_RJP);
	}

	char *table = print_table_names_from_query(queryDesc);
	int table_index = -1;
	
	if(table){
		table_index = string_vector_find(tables, table);

		if (table_index == -1){
			string_vector_push(tables, pstrdup(table));
			bool_vector_push(built_pointers_vec, false);
			bool_vector_push(reset_outputArrays_vec, true);
			PtrVector *new_table_pointers = (PtrVector *) malloc(sizeof(PtrVector));
			StringVector *new_table_attributes = (StringVector *) malloc(sizeof(StringVector));
			PtrVector *new_table_prefix_sums = (PtrVector *) malloc(sizeof(PtrVector));
			PtrVector *new_table_distinct_colors = (PtrVector *) malloc(sizeof(PtrVector));
			PtrVector *new_table_LJP = (PtrVector *) malloc(sizeof(PtrVector));
			PtrVector *new_table_RJP = (PtrVector *) malloc(sizeof(PtrVector));
			PtrVector *new_table_weighted_pointers = (PtrVector *) malloc(sizeof(PtrVector));

			ptr_vector_init(new_table_pointers);
			string_vector_init(new_table_attributes);
			ptr_vector_init(new_table_prefix_sums);
			ptr_vector_init(new_table_distinct_colors);
			ptr_vector_init(new_table_LJP);
			ptr_vector_init(new_table_RJP);
			ptr_vector_init(new_table_weighted_pointers);

			ptr_vector_push(table_pointers, (void *)new_table_pointers);
			ptr_vector_push(table_attributes, (void *)new_table_attributes);
			ptr_vector_push(table_prefix_sums, (void *)new_table_prefix_sums);
			ptr_vector_push(table_distinct_colors, (void *)new_table_distinct_colors);
			ptr_vector_push(table_LJP, (void *)new_table_LJP);
			ptr_vector_push(table_RJP, (void *)new_table_RJP);
			ptr_vector_push(table_weighted_pointers, (void *)new_table_weighted_pointers);

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
			table_prefix_sums->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));
			table_distinct_colors->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));
			table_LJP->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));
			table_RJP->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));
			table_weighted_pointers->data[table_index] = (PtrVector *) malloc(sizeof(PtrVector));

			ptr_vector_init(table_pointers->data[table_index]);
			string_vector_init(table_attributes->data[table_index]);
			ptr_vector_init(table_prefix_sums->data[table_index]);
			ptr_vector_init(table_distinct_colors->data[table_index]);
			ptr_vector_init(table_LJP->data[table_index]);
			ptr_vector_init(table_RJP->data[table_index]);
			ptr_vector_init(table_weighted_pointers->data[table_index]);

			reset_outputArrays_vec->data[table_index] = false;
		}

		// }
		// else
		// {
		// 	elog(INFO, "Top-level planstate is not a scan node (type: %d)", nodeTag(ps));
		// }
		Plan *plan;
		plan = queryDesc->planstate->plan;
		Oid relid = InvalidOid;

		PlanState *ps = queryDesc->planstate;

		// if (is_scan_node((Node *) ps))  // use the is_scan_node helper from before
		// {
		ScanState *scanstate = (ScanState *) ps;
		Scan *scan = (Scan *) scanstate->ps.plan;

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
						
						if (constant->consttype == FLOAT8OID)		bounds[n_bounds++] = DatumGetFloat8(constant->constvalue);
						else if (constant->consttype == INT4OID)	bounds[n_bounds++] = (float)DatumGetInt32(constant->constvalue);
						else 										elog(ERROR, "Unsupported constant type");
						
						// Optional: get the operator itself
						// elog(INFO, "Operator OID: %u", opexpr->opno);
					}
				}
			}
		}
	
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

		sort_attr_index = get_attribute_index(column_header->data, vec->natts, attribute);
		color_index = get_attribute_index(column_header->data, vec->natts, subjectToStmt->attr);
		attr_index = string_vector_find(table_attributes->data[table_index], attribute);
		epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
		num_tuples = vec -> size;
		MAX_ATTRS = column_header->natts;
		int **prefix_sums;
		StringVector *color_vector;
		
		if(!built_pointers_vec->data[table_index] || attr_index == -1){
			
			outputArray = (char ***)malloc(vec->size * sizeof(char **));
						
			// elog(INFO, "Output vector size: %d", vec->size);
			for (int i = 0; i < vec->size; i++) {
				outputArray[i] = (char **)malloc(vec->natts * sizeof(char *));
				for (int j = 0; j < vec->natts; j++) {
					outputArray[i][j] = strdup(vec->data[i][j]);
				}
			}

			merge_sort(0, num_tuples - 1, sort_attr_index);

			int color_count = 0;
			char **colors = compute_distinct_colors(num_tuples, color_index, &color_count);

			color_vector = (StringVector *) malloc(sizeof(StringVector));

			string_vector_init(color_vector);
			
			for (int i = 0; i < color_count; i++)
			{
				string_vector_push(color_vector, colors[i]);
			}
			prefix_sums = compute_prefix_sums(num_tuples, colors, color_count, color_index);

			if (attr_index != -1) {
				ptr_vector_replace(table_pointers->data[table_index], attr_index, (void *)outputArray);
				ptr_vector_replace(table_prefix_sums->data[table_index], attr_index, (void *)prefix_sums);
				ptr_vector_replace(table_distinct_colors->data[table_index], attr_index, (void *)color_vector);
			} else {
				ptr_vector_push(table_pointers->data[table_index], (void *)outputArray);
				ptr_vector_push(table_prefix_sums->data[table_index], (void *)prefix_sums);
				ptr_vector_push(table_distinct_colors->data[table_index], (void *)color_vector);
				string_vector_push(table_attributes->data[table_index], attribute);
			}
		}
		else{
			outputArray = (char ***)ptr_vector_get(table_pointers->data[table_index], attr_index);
			prefix_sums = (int **)ptr_vector_get(table_prefix_sums->data[table_index], attr_index);
			color_vector = (StringVector *)ptr_vector_get(table_distinct_colors->data[table_index], attr_index);
		}

		l_index = get_upper_value_index(index, bounds[0]) + 1;
		r_index = get_lower_value_index(index, bounds[1]) + 1;

		Range output;
		
		
		bool fair = fairness_check(subjectToStmt, color_vector, prefix_sums, l_index-1, r_index-1); // 0-indexing

		if(fair){
			elog(INFO, "FAIR QUERY OUTPUT");
		}
		else{
			elog(INFO, "UNFAIR QUERY OUTPUT");
		}
		
		if(!fair){
			if(list_length(subjectToStmt->attr_list) == 2 && is_binary_attr(vec, get_attribute_index(column_header->data, vec->natts, subjectToStmt->attr))){
				// elog(INFO, "Binary attribute");
				RBTree *rbt;

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
					w2 = 1;
				} else {
					w2 = ((A_Const*)item1->into_val)->val.ival.ival;
				}
				
				bool switch_ptrs = false;

				if (strcmp(item0->attr_name, item1->attr_name) > 0) {
					int temp = w1;
					w1 = w2;
					w2 = temp;
					switch_ptrs = true;
				}

				if(!built_pointers_vec->data[table_index] || attr_index == -1){

				
					buildingpointers_w(column_header->data, attribute, vec->natts, subjectToStmt);
					
					weighted_pointers_node = (WeightedPointersRBNode *) malloc(sizeof(WeightedPointersRBNode));
					weighted_pointers_node->weight1 = w1;
					weighted_pointers_node->weight2 = w2;
					weighted_pointers_node->fwdNegPtr = fwdNegPtr;
					weighted_pointers_node->fwdPosPtr = fwdPosPtr;
					weighted_pointers_node->prevNegPtr = prevNegPtr;
					weighted_pointers_node->prevPosPtr = prevPosPtr;

					built_pointers_vec->data[table_index] = true;
					// elog(INFO, "Built pointers");	
					if (weightedPointersContext == NULL) {
						weightedPointersContext = AllocSetContextCreate(
							TopMemoryContext,         // Use this for global/session-lifetime allocations
							"MySessionMemoryContext",
							ALLOCSET_DEFAULT_SIZES
						);
					}
					
					MemoryContext oldContext = MemoryContextSwitchTo(weightedPointersContext);

					// Perform operations within the new memory context

					rbt = rbt_create(sizeof(WeightedPointersRBNode),   /* Node size */
					weighted_pointers_rbtree_comparator, /* Comparator */
					weighted_pointers_rbtree_combiner,   /* Combiner */
					weighted_pointers_rbtree_allocfunc,  /* Allocator */
					weighted_pointers_rbtree_freefunc,   /* Free function */
					NULL);
					
					// elog(INFO, "Created RBTree");

					bool newinsert;
					rbt_insert(rbt, (RBTNode *) weighted_pointers_node, &newinsert);

					if (!newinsert) {
						elog(ERROR, "Failed to insert into RBTree");
					}

					if (attr_index != -1) {
						// ptr_vector_replace(table_LJP->data[table_index], attr_index, (void *)LJP);
						// ptr_vector_replace(table_RJP->data[table_index], attr_index, (void *)RJP);
						ptr_vector_replace(table_weighted_pointers->data[table_index], attr_index, (void *)rbt);
					} else {
						// ptr_vector_push(table_LJP->data[table_index], (void *)LJP);
						// ptr_vector_push(table_RJP->data[table_index], (void *)RJP);
						ptr_vector_push(table_weighted_pointers->data[table_index], (void *)rbt);
						// elog(INFO, "pushed attribute: %s", attribute);
					}
					// elog(INFO, "Pushed attribute: %s", attribute);
					MemoryContextSwitchTo(oldContext);
				}
				else{
					// elog(INFO, "Using existing pointers");
					// LJP = (int *)ptr_vector_get(table_LJP->data[table_index], attr_index);
					// RJP = (int *)ptr_vector_get(table_RJP->data[table_index], attr_index);
					
					rbt = (RBTree *)ptr_vector_get(table_weighted_pointers->data[table_index], attr_index);
					
					WeightedPointersRBNode* node = (WeightedPointersRBNode *) malloc(sizeof(WeightedPointersRBNode));
					node->weight1 = w1;
					node->weight2 = w2;
					weighted_pointers_node = (WeightedPointersRBNode *) rbt_find(rbt, (RBTNode *) node);

					if (weighted_pointers_node == NULL) {
						buildingpointers_w(column_header->data, attribute, vec->natts, subjectToStmt);
					
						weighted_pointers_node = (WeightedPointersRBNode *) malloc(sizeof(WeightedPointersRBNode));
						weighted_pointers_node->weight1 = w1;
						weighted_pointers_node->weight2 = w2;
						weighted_pointers_node->fwdNegPtr = fwdNegPtr;
						weighted_pointers_node->fwdPosPtr = fwdPosPtr;
						weighted_pointers_node->prevNegPtr = prevNegPtr;
						weighted_pointers_node->prevPosPtr = prevPosPtr;

						// elog(INFO, "Inserted new node");
						bool newinsert;
						rbt_insert(rbt, (RBTNode *) weighted_pointers_node, &newinsert);

					}
					else{
						// elog(INFO, "Found existing node");
					}

					if (switch_ptrs) {
						fwdNegPtr = weighted_pointers_node->fwdPosPtr;
						fwdPosPtr = weighted_pointers_node->fwdNegPtr;
						prevNegPtr = weighted_pointers_node->prevPosPtr;
						prevPosPtr = weighted_pointers_node->prevNegPtr;
					}
					else{					
						fwdPosPtr = weighted_pointers_node->fwdPosPtr;
						fwdNegPtr = weighted_pointers_node->fwdNegPtr;
						prevPosPtr = weighted_pointers_node->prevPosPtr;
						prevNegPtr = weighted_pointers_node->prevNegPtr;
					}
				}

				elog(INFO, "l_index: %d, r_index: %d", l_index, r_index);
				output = getrange(column_header->data, vec->natts, subjectToStmt, l_index, r_index, epsilon);

				// elog(INFO, "Fair range: [%d, %d]", output.start, output.end);
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
			else{
				// elog(INFO, "Non-binary attribute");

				elog(INFO, "l_index: %d, r_index: %d", l_index, r_index);
				epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
				output = multicolor_getrange(vec, column_header->data, attribute, vec->natts, subjectToStmt, l_index, r_index, epsilon);
				elog(INFO, "Naive Fair range: [%d, %d]", output.start, output.end);
				//%jalu writing this, do not hit
				Range originalrange = createRange(l_index, r_index);
				Range recursivebfsoutput = recursivebfs(originalrange, color_vector, prefix_sums, subjectToStmt);
				elog(INFO, "BFSRecursive Fair range: [%d, %d]", recursivebfsoutput.start, recursivebfsoutput.end);
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
					elog(INFO, "NAIVE CORRECTED QUERY:");
					elog(INFO, "SELECT * FROM %s WHERE %s BETWEEN %s AND %s", table, attribute, outputArray[output.start - 1][index], outputArray[output.end - 1][index]);
					elog(INFO, "BFS CORRECTED QUERY:");
					elog(INFO, "SELECT * FROM %s WHERE %s BETWEEN %s AND %s", table, attribute, outputArray[recursivebfsoutput.start - 1][index], outputArray[recursivebfsoutput.end - 1][index]);
				}
			}
		}
    }
}


StringVector* compute_headers(QueryDesc* queryDesc){
    StringVector *column_header;
    char* attribute;
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
    return column_header;
}


Range createRange(int st, int e) {
	Range range;
	range.start = st;
	range.end = e;
	return range;
}

bool fairness_check_naive(char **column_headers, CharPtrVector* vec,  SubjectToStmt* subjectToStmt, float lower_bound, float upper_bound, int col_index)
{
	int num_colors = list_length(subjectToStmt->attr_list);

	int Wr, Wb, epsilon;

	int *weights = (int *) palloc(num_colors * sizeof(int));

	for (int i = 0; i < num_colors; i++)
	{
		AttrWithInto *item = (AttrWithInto *) list_nth(subjectToStmt->attr_list, i);
		if(item->into_val != NULL){
			weights[i] = ((A_Const*)item->into_val)->val.ival.ival;
		}
		else{
			weights[i] = 1;
		}
	}

	int natts = vec->natts;
	int idx;

	char **C = (char **) palloc(num_colors * sizeof(char *));
	
	for (int i = 0; i < num_colors; i++)
	{
		AttrWithInto *item = (AttrWithInto *) list_nth(subjectToStmt->attr_list, i);
		C[i] = item->attr_name;
	}

	char *column_name = subjectToStmt->attr;
	epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;
	idx = get_attribute_index(column_headers, natts, column_name);

	int *count_features = (int *) palloc(num_colors * sizeof(int));
	for (int i = 0; i < num_colors; i++)
	{
		count_features[i] = 0;
	}
	
	for(int i = 0; i<vec->size;i++)
	{
		if(atof(vec->data[i][col_index]) > upper_bound+0.00001 || atof(vec->data[i][col_index]) < lower_bound-0.00001)
		{
			continue;
		}
		for (int j = 0; j < num_colors; j++)
		{
			if (strcmp(vec->data[i][idx], C[j]) == 0)
			{
				count_features[j]++;
			}
		}
	}

	bool fair = true;

	for (int i = 0; i < num_colors; i++)
	{
		for (int j = 0; j < num_colors; j++)
		{
			if(i != j)
			{
				if (abs(count_features[i]*weights[i] - count_features[j]*weights[j]) > epsilon)
				{
					fair = false;
				}
			}
		}
	}
	
	if(fair)
	{
		elog(INFO, "FAIR QUERY OUTPUT");
	}
	else
	{
		elog(INFO, "UNFAIR QUERY OUTPUT");
	}

	return fair;
}

bool fairness_check(SubjectToStmt* subjectToStmt, StringVector* color_vector, int **prefixsum, int start, int end)
{
	int num_colors = list_length(subjectToStmt->attr_list);

	int epsilon;

	int *weights = (int *) palloc(num_colors * sizeof(int));

	for (int i = 0; i < num_colors; i++)
	{
		AttrWithInto *item = (AttrWithInto *) list_nth(subjectToStmt->attr_list, i);
		if(item->into_val != NULL){
			weights[i] = ((A_Const*)item->into_val)->val.ival.ival;
		}
		else{
			weights[i] = 1;
		}
	}

	char **C = (char **) palloc(num_colors * sizeof(char *));
	
	for (int i = 0; i < num_colors; i++)
	{
		AttrWithInto *item = (AttrWithInto *) list_nth(subjectToStmt->attr_list, i);
		C[i] = item->attr_name;
	}

	char *column_name = subjectToStmt->attr;
	epsilon = ((A_Const*)subjectToStmt->threshold_val)->val.ival.ival;

	bool fair = true;

	for (int i = 0; i < num_colors; i++)
	{
		int color_idx1;
		for (int k = 0; k < color_vector->size; k++)
		{
			if (strcmp(color_vector->data[k], C[i]) == 0)
			{
				color_idx1 = k;
			}
		}
		for (int j = 0; j < num_colors; j++)
		{
			int color_idx2;
			for (int k = 0; k < color_vector->size; k++)
			{
				if (strcmp(color_vector->data[k], C[j]) == 0)
				{
					color_idx2 = k;
				}
			}
			if(i != j)
			{
				if (start != 0){
					if (abs(weights[i]*(prefixsum[color_idx1][end] - prefixsum[color_idx1][start-1]) - weights[j]*(prefixsum[color_idx2][end] - prefixsum[color_idx2][start-1])) > epsilon)
					{
						fair = false;
					}
				}
				else{
					if (abs(weights[i]*(prefixsum[color_idx1][end]) - weights[j]*(prefixsum[color_idx2][end])) > epsilon)
					{
						fair = false;
					}
				}
			}
		}
	}

	return fair;
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
	int left = 0, right = num_tuples - 1;
	int nearest_index = -1;

	while (left <= right) {
		int mid = left + (right - left) / 2;
		float current_value = atof(outputArray[mid][index]);

		if (current_value <= value) {
			nearest_index = mid;
			left = mid + 1; // Search in the right half
		} else {
			right = mid - 1; // Search in the left half
		}
	}

	if (nearest_index != -1) {
		return nearest_index;
	}

	elog(ERROR, "No value <= %f found in the relation", value);
	return -1; // No value <= value found
}

int get_upper_value_index(int index, float value) {
	int left = 0, right = num_tuples - 1;
	int nearest_index = -1;

	while (left <= right) {
		int mid = left + (right - left) / 2;
		float current_value = atof(outputArray[mid][index]);

		if (current_value >= value) {
			nearest_index = mid;
			right = mid - 1; // Search in the left half
		} else {
			left = mid + 1; // Search in the right half
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

char **compute_distinct_colors(int num_tuples, int color_index, int *color_count){
	char **distinct_colors = (char **) palloc(num_tuples * sizeof(char *));
	int count = 0;
	for (int i = 0; i < num_tuples; i++) {
		bool found = false;
		for (int j = 0; j < count; j++) {
			if (my_strcmp(distinct_colors[j], outputArray[i][color_index]) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			distinct_colors[count] = outputArray[i][color_index];
			count++;
		}
	}
	*color_count = count;
	return distinct_colors;
}

int **compute_prefix_sums(int num_tuples, char **distinct_colors, int color_count, int color_index) {
	int **prefix_sums = (int **) malloc(color_count * sizeof(int *));
	for (int i = 0; i < color_count; i++) {
		prefix_sums[i] = (int *) malloc((num_tuples + 1) * sizeof(int));
	}

	for (int i = 0; i < color_count; i++) {
		for (int j = 0; j <= num_tuples; j++) {
			prefix_sums[i][j] = 0;
		}
	}

	for (int i = 0; i < num_tuples; i++) {
		for (int j = 0; j < color_count; j++) {
			if (i > 0) {
				if (my_strcmp(outputArray[i][color_index], distinct_colors[j]) == 0) {
					prefix_sums[j][i] = prefix_sums[j][i-1] + 1;
				} else {
					prefix_sums[j][i] = prefix_sums[j][i-1];
				}
			}
			else {
				if (my_strcmp(outputArray[i][color_index], distinct_colors[j]) == 0) {
					prefix_sums[j][i] = 1;
				} else {
					prefix_sums[j][i] = 0;
				}
			}
		}
	}

	return prefix_sums;
	
}

typedef struct heapnode
{
	struct heapnode *first_child;
	struct heapnode *next_sibling;
	struct heap_node *prev_or_parent;
	Range r;
	float similarity;
	
} heapnode;
int int_rbtree_comparator(const RBTNode *a, const RBTNode *b, void *arg)
{
	const IntRBNode *nodeA = (const IntRBNode *) a;
	const IntRBNode *nodeB = (const IntRBNode *) b;

	if (nodeA->key < nodeB->key)
		return -1;
	else if (nodeA->key > nodeB->key)
		return 1;
	return 0;
}
int weighted_pointers_rbtree_comparator(const RBTNode *a, const RBTNode *b, void *arg)
{
	const WeightedPointersRBNode *nodeA = (const WeightedPointersRBNode *) a;
	const WeightedPointersRBNode *nodeB = (const WeightedPointersRBNode *) b;

	if (nodeA->weight1 < nodeB->weight1 || (nodeA->weight1 == nodeB->weight1 && nodeA->weight2 < nodeB->weight2))
		return -1;
	else if (nodeA->weight1 > nodeB->weight1 || (nodeA->weight1 == nodeB->weight1 && nodeA->weight2 > nodeB->weight2))
		return 1;
	return 0;
}
int heapnode_comparator(const pairingheap_node *a,
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
void int_rbtree_combiner(RBTNode *existing, const RBTNode *newdata, void *arg)
{
    IntRBNode *existNode = (IntRBNode *) existing;
    const IntRBNode *newNode = (const IntRBNode *) newdata;
    /* Overwrite existing value */
   
}
void weighted_pointers_rbtree_combiner(RBTNode *existing, const RBTNode *newdata, void *arg)
{
	WeightedPointersRBNode *existNode = (WeightedPointersRBNode *) existing;
	const WeightedPointersRBNode *newNode = (const WeightedPointersRBNode *) newdata;
	/* Overwrite existing value */
   
}

RBTNode *int_rbtree_allocfunc(void *arg)
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
RBTNode *weighted_pointers_rbtree_allocfunc(void *arg)
{
	WeightedPointersRBNode *newNode = (WeightedPointersRBNode *) malloc(sizeof(WeightedPointersRBNode));
	newNode->weight1 = 0;
	newNode->weight2 = 0;
	newNode->fwdPosPtr = (int *) malloc((num_tuples + 2) * sizeof(int));
	newNode->fwdNegPtr = (int *) malloc((num_tuples + 2) * sizeof(int));
	newNode->prevPosPtr = (int *) malloc((num_tuples + 2) * sizeof(int));
	newNode->prevNegPtr = (int *) malloc((num_tuples + 2) * sizeof(int));
	if (!newNode->fwdPosPtr || !newNode->fwdNegPtr || !newNode->prevPosPtr || !newNode->prevNegPtr) {
		perror("Failed to allocate values");
		exit(EXIT_FAILURE);
	}
	return (RBTNode *) newNode;
}

void int_rbtree_freefunc(RBTNode *node, void *arg)
{
    pfree(node);
}
void weighted_pointers_rbtree_freefunc(RBTNode *node, void *arg)
{
	WeightedPointersRBNode *node1 = (WeightedPointersRBNode *) node;
	pfree(node1->fwdPosPtr);
	pfree(node1->fwdNegPtr);
	pfree(node1->prevPosPtr);
	pfree(node1->prevNegPtr);
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
	
	RBTree* tree = rbt_create(sizeof(IntRBNode),   /* Node size */
                          int_rbtree_comparator, /* Comparator */
                          int_rbtree_combiner,   /* Combiner */
                          int_rbtree_allocfunc,  /* Allocator */
                          int_rbtree_freefunc,   /* Free function */
                          NULL); 
    cumulative = 0;
    RJP = (int *)malloc((num_tuples + 2) * sizeof(int));
	LJP = (int *)malloc((num_tuples + 2) * sizeof(int));
    int *c = (int *)malloc((num_tuples + 2) * sizeof(int));
	int *color = (int *)malloc((num_tuples + 2) * sizeof(int));
	
    for (int i = 0; i < num_tuples + 2; i++) 
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
			color[i+1] = -1;
        } else if (my_strcmp(outputArray[i][attr_index], feature1) == 0) {
			color[i+1] = 1;
        }
	}
	// elog(INFO,"color[7]:%d",color[7]);
	c[0] = 0;
	c[num_tuples + 1] = 0;
	color[0] = 0;
	color[num_tuples + 1] = 0;
	
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
	c[num_tuples + 1] = 0;
	cumulative=0;
	// elog(INFO, "Printing right jump pointers:\n");
    // for (int i = 0; i < num_tuples+1; i++) {
    //     elog(INFO, "RJP[%d] = %d\n", i, RJP[i]);
    // }
	for (int i = num_tuples + 1; i >= 1; i--) {
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
	// elog(INFO,"NO ERROR IN JUMP POINTERS");
	// elog(INFO, "Printing left jump pointers:\n");
	// for (int i = 0; i < num_tuples+2; i++) {
	// 	elog(INFO, "LJP[%d] = %d\n", i, LJP[i]);
	// }
	// elog(INFO,"checkin jp function");
	// elog(INFO,"%d",jp(column_headers,sourceText,natts,attribute,13,-1,"left",LJP,RJP,color));
    return;
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
Range getrange_LJP_RJP(char **column_headers, int natts, SubjectToStmt* subjectToStmt, int start, int end, int epsilon)
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

	char* feature1 = item0->attr_name;
	char* feature2 = item1->attr_name;

    for (int i = 0; i < num_tuples+1; i++) {
		if(i>=1 && i<=num_tuples)
		{
			if (my_strcmp(outputArray[i-1][attr_index], feature1) == 0) {
				cumulative += 1;
			} else if (my_strcmp(outputArray[i-1][attr_index], feature2) == 0) {
				cumulative -= 1;
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
	double best_similarity = 0;
	Range fair_range = createRange(start,end);
	int disparitydifference =abs(disparity) - epsilon;
	int jumps = disparitydifference;
	mystack startleft, startright, endleft, endright;
	initializestack(&startleft, "startleft");
	initializestack(&startright, "startright");
	initializestack(&endleft, "endleft");
	initializestack(&endright, "endright");
	int excesscolor = disparity > 0 ? 1 : -1;
	pushstack(&startleft, start);
	pushstack(&startright, start);
	pushstack(&endleft, end);
	pushstack(&endright, end);
	while(jumps)
	{
		int topindex = top(&startright);
		if(color[topindex] == excesscolor)
		{
			pushstack(&startright, topindex+1);
		}
		else 
		{
			pushstack(&startright,RJP[topindex-1]+1);
		}
		--jumps;
	}
	jumps = disparitydifference;
	while(jumps)
	{
		int sr = top(&startright);
		int el = top(&endleft);
		int er = top(&endright);
		Range sr_el = createRange(sr,el);
		double similarity_sr_el = jaccordsimilarity(createRange(start,end), sr_el);

		if(similarity_sr_el > best_similarity)
		{
			best_similarity = similarity_sr_el;
			fair_range = sr_el;			
		}
		if(size(&endright)+size(&startright)==disparitydifference+2)
		{
			Range sr_er = createRange(sr,er);
			double similarity_sr_er = jaccordsimilarity(createRange(start,end), sr_er);
			if(similarity_sr_er > best_similarity)
			{
				best_similarity = similarity_sr_er;
				fair_range = sr_er;
			}
		}

		if(er<num_tuples +1)
		{
			if(color[er+1] != excesscolor)
			{
				pushstack(&endright,er+1);
			}
			else if(RJP[er]!=-1)
			{
				pushstack(&endright,RJP[er]);
			}
		}
		if(color[el]==excesscolor)
		{
			pushstack(&endleft,el-1);
		}
		else
		{
			pushstack(&endleft,LJP[el+1]-1);
		}
		pop(&startright);
		--jumps;

	}
	jumps = disparitydifference+1;

	while(jumps)
	{
		int sl = top(&startleft);
		int el = top(&endleft);
		int er = top(&endright);
		Range sl_el = createRange(sl,el);
		double similarity_sl_el = jaccordsimilarity(createRange(start,end), sl_el);
		if(similarity_sl_el > best_similarity)
		{
			best_similarity = similarity_sl_el;
			fair_range = sl_el;
		}
		dlist_iter iter;
		dlist_foreach(iter, &startleft.header)
		{
			mynode *node = dlist_container(mynode, node, iter.cur);
		}

		dlist_foreach(iter, &endleft.header)
		{
			mynode *node = dlist_container(mynode, node, iter.cur);
		}
		if(size(&endright)+size(&startleft)==disparitydifference+2)
		{
			Range sl_er = createRange(sl,er);
			double similarity_sl_er = jaccordsimilarity(createRange(start,end), sl_er);
			if(similarity_sl_er > best_similarity)
			{
				best_similarity = similarity_sl_er;
				fair_range = sl_er;
			}
			pop(&endright);
		}
		pop(&endleft);
		if(sl>1)
		{
			if(color[sl-1] != excesscolor)
			{
				pushstack(&startleft,sl-1);
			}
			else if(LJP[sl]!=-1)
			{
				pushstack(&startleft,LJP[sl]);
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
		--jumps;
	}
	elog(INFO,"Best similarity: %.2f",best_similarity);
	
    free(c);

	return fair_range;
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


bool
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
	if(r.start < 1 || r.end < 1 || r.start > num_tuples || r.end > num_tuples)
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

Range recursivebfs(Range originalrange, StringVector* color_vector, int **prefix_sum, SubjectToStmt* subjectToStmt)
{
	pairingheap *currheap = pairingheap_allocate(heapnode_comparator, NULL);
	heapnode *first_node = (heapnode *) palloc(sizeof(heapnode));
	first_node->r = originalrange;
	first_node->similarity = jaccordsimilarity(originalrange, originalrange);
	pairingheap_add(currheap, (pairingheap_node *) first_node);	
	
	// elog(INFO, "Original Range: [%d, %d]", originalrange.start, originalrange.end);
	// elog(INFO, "Original Similarity: %.2f", first_node->similarity);

	while(true)
	{
		if(pairingheap_is_empty(currheap))
		{
			break;
		}
		heapnode *maxsimilarity_node = (heapnode *) pairingheap_remove_first(currheap);
		bool topsimilarityfair = fairness_check(subjectToStmt, color_vector, prefix_sum, maxsimilarity_node->r.start-1, maxsimilarity_node->r.end-1);
		// elog(INFO, "Range: [%d, %d]", maxsimilarity_node->r.start, maxsimilarity_node->r.end);
		// elog(INFO, "Similarity: %.2f", maxsimilarity_node->similarity);
		if(topsimilarityfair)
		{
			elog(INFO, "BFSRecursive Best Similarity: %.2f", maxsimilarity_node->similarity);
			return maxsimilarity_node->r;
		}
		int topstart = maxsimilarity_node->r.start;
		int topend = maxsimilarity_node->r.end;
		int topstartminus = topstart - 1;
		int topendplus = topend + 1;
		int topstartplus = topstart + 1;
		int topendminus = topend - 1;
		Range newrange1 = createRange(topstartminus, topend);
		Range newrange2 = createRange(topstartplus, topend);
		Range newrange3 = createRange(topstart, topendminus);
		Range newrange4 = createRange(topstart, topendplus);
		if(validrange(newrange1))
		{
			heapnode *new_node1 = (heapnode *) palloc(sizeof(heapnode));
			new_node1->r = newrange1;
			new_node1->similarity = jaccordsimilarity(originalrange, newrange1);
			if(new_node1->similarity < maxsimilarity_node->similarity)pairingheap_add(currheap, (pairingheap_node *) new_node1);
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
	return createRange(-1, -1);
}

bool is_binary_attr(CharPtrVector* vec, int index){
	if (vec->size < 2) {
		return false;
	}

	char *value1 = NULL;
	char *value2 = NULL;

	for (int i = 0; i < vec->size; i++) {
		char *current_value = vec->data[i][index];

		if (value1 == NULL) {
			value1 = current_value;
		} else if (strcmp(current_value, value1) != 0) {
			if (value2 == NULL) {
				value2 = current_value;
			} else if (strcmp(current_value, value2) != 0) {
				return false; // More than 2 unique values found
			}
		}
	}

	return true; // Only 2 unique values found
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
			node = (IntRBNode*) rbt_iterate(iter);
		}
		node = (IntRBNode *) palloc(sizeof(IntRBNode));
		node->key = c[i];
		node = (IntRBNode*) rbt_find_great(fwdNegBST, (RBTNode*) node, 0);
		rbt_begin_iterate_from(fwdNegBST, LeftRightWalk, iter, (RBTNode*) node);
		while (node) {
			for (int j = node->start; j < node->index; j++) {
				fwdNegPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = (IntRBNode*) rbt_iterate(iter);
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
			node = (IntRBNode*) rbt_iterate(iter);
		}
		node = (IntRBNode *) palloc(sizeof(IntRBNode));
		node->key = c[i];
		node = (IntRBNode*) rbt_find_great(prevNegBST, (RBTNode*) node, 0);
		rbt_begin_iterate_from(prevNegBST, LeftRightWalk, iter, (RBTNode*) node);
		while (node) {
			for (int j = node->start; j < node->index; j++) {
				prevNegPtr[node->values[j]] = i;
			}
			node->start = node->index;
			node = (IntRBNode*) rbt_iterate(iter);
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