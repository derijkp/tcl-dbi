
#define STRING 1
#define BITMASK 2
#define UBITMASK 3
#define VALUE 4
#define VALUE32 5
#define ENUM 6

typedef struct Getinfo_values {
	unsigned int code;
	char *name;
} Getinfo_values;

typedef struct Getinfo_cor {
	char *name;
	unsigned int code;
	int type;
	Getinfo_values *values;
} Getinfo_cor;

Getinfo_values getinfo_values_aggregate_functions[] = {
	{(unsigned int)SQL_AF_ALL,"all"},
	{(unsigned int)SQL_AF_AVG,"avg"},
	{(unsigned int)SQL_AF_COUNT,"count"},
	{(unsigned int)SQL_AF_DISTINCT,"distinct"},
	{(unsigned int)SQL_AF_MAX,"max"},
	{(unsigned int)SQL_AF_MIN,"min"},
	{(unsigned int)SQL_AF_SUM,"sum"},
	{0,NULL},
};

Getinfo_values getinfo_values_alter_domain[] = {
	{(unsigned int)SQL_AD_ADD_DOMAIN_CONSTRAINT,"add_domain_constraint"},
	{(unsigned int)SQL_AD_ADD_DOMAIN_DEFAULT,"add_domain_default"},
	{(unsigned int)SQL_AD_CONSTRAINT_NAME_DEFINITION,"constraint_name_definition"},
	{(unsigned int)SQL_AD_DROP_DOMAIN_CONSTRAINT,"drop_domain_constraint"},
	{(unsigned int)SQL_AD_DROP_DOMAIN_DEFAULT,"drop_domain_default"},
	{(unsigned int)SQL_AD_ADD_CONSTRAINT_DEFERRABLE,"add_constraint_deferrable"},
	{(unsigned int)SQL_AD_ADD_CONSTRAINT_NON_DEFERRABLE,"add_constraint_non_deferrable"},
	{(unsigned int)SQL_AD_ADD_CONSTRAINT_INITIALLY_DEFERRED,"add_constraint_initially_deferred"},
	{(unsigned int)SQL_AD_ADD_CONSTRAINT_INITIALLY_IMMEDIATE,"add_constraint_initially_immediate"},
	{0,NULL},
};

Getinfo_values getinfo_values_alter_table[] = {
	{(unsigned int)SQL_AT_ADD_COLUMN_COLLATION,"add_column_collation"},
	{(unsigned int)SQL_AT_ADD_COLUMN_DEFAULT,"add_column_default"},
	{(unsigned int)SQL_AT_ADD_COLUMN_SINGLE,"add_column_single"},
	{(unsigned int)SQL_AT_ADD_CONSTRAINT,"add_constraint"},
	{(unsigned int)SQL_AT_ADD_TABLE_CONSTRAINT,"add_table_constraint"},
	{(unsigned int)SQL_AT_CONSTRAINT_NAME_DEFINITION,"constraint_name_definition"},
	{(unsigned int)SQL_AT_DROP_COLUMN_CASCADE,"drop_column_cascade"},
	{(unsigned int)SQL_AT_DROP_COLUMN_DEFAULT,"drop_column_default"},
	{(unsigned int)SQL_AT_DROP_COLUMN_RESTRICT,"drop_column_restrict"},
	{(unsigned int)SQL_AT_DROP_TABLE_CONSTRAINT_CASCADE,"drop_table_constraint_cascade"},
	{(unsigned int)SQL_AT_DROP_TABLE_CONSTRAINT_RESTRICT,"drop_table_constraint_restrict"},
	{(unsigned int)SQL_AT_SET_COLUMN_DEFAULT,"set_column_default"},
	{(unsigned int)SQL_AT_CONSTRAINT_INITIALLY_DEFERRED,"constraint_initially_deferred"},
	{(unsigned int)SQL_AT_CONSTRAINT_INITIALLY_IMMEDIATE,"constraint_initially_immediate"},
	{(unsigned int)SQL_AT_CONSTRAINT_DEFERRABLE,"constraint_deferrable"},
	{(unsigned int)SQL_AT_CONSTRAINT_NON_DEFERRABLE,"constraint_non_deferrable"},
	{0,NULL},
};

Getinfo_values getinfo_values_async_mode[] = {
	{(unsigned int)SQL_AM_CONNECTION,"connection"},
	{(unsigned int)SQL_AM_STATEMENT,"statement"},
	{(unsigned int)SQL_AM_NONE,"none"},
	{0,NULL},
};

Getinfo_values getinfo_values_batch_row_count[] = {
	{(unsigned int)SQL_BRC_ROLLED_UP,"rolled_up"},
	{(unsigned int)SQL_BRC_PROCEDURES,"procedures"},
	{(unsigned int)SQL_BRC_EXPLICIT,"explicit"},
	{0,NULL},
};

Getinfo_values getinfo_values_batch_support[] = {
	{(unsigned int)SQL_BS_SELECT_EXPLICIT,"select_explicit"},
	{(unsigned int)SQL_BS_ROW_COUNT_EXPLICIT,"row_count_explicit"},
	{(unsigned int)SQL_BS_SELECT_PROC,"select_proc"},
	{(unsigned int)SQL_BS_ROW_COUNT_PROC,"row_count_proc"},
	{0,NULL},
};

Getinfo_values getinfo_values_bookmark_persistence[] = {
	{(unsigned int)SQL_BP_CLOSE,"close"},
	{(unsigned int)SQL_BP_DELETE,"delete"},
	{(unsigned int)SQL_BP_DROP,"drop"},
	{(unsigned int)SQL_BP_TRANSACTION,"transaction"},
	{(unsigned int)SQL_BP_UPDATE,"update"},
	{(unsigned int)SQL_BP_OTHER_HSTMT,"other_hstmt"},
	{0,NULL},
};

Getinfo_values getinfo_values_catalog_location[] = {
	{(unsigned int)SQL_CL_START,"start"},
	{(unsigned int)SQL_CL_END,"end"},
	{0,NULL},
};

Getinfo_values getinfo_values_catalog_usage[] = {
	{(unsigned int)SQL_CU_DML_STATEMENTS,"dml_statements"},
	{(unsigned int)SQL_CU_PROCEDURE_INVOCATION,"procedure_invocation"},
	{(unsigned int)SQL_CU_TABLE_DEFINITION,"table_definition"},
	{(unsigned int)SQL_CU_INDEX_DEFINITION,"index_definition"},
	{(unsigned int)SQL_CU_PRIVILEGE_DEFINITION,"privilege_definition"},
	{0,NULL},
};

Getinfo_values getinfo_values_concat_null_behavior[] = {
	{(unsigned int)SQL_CB_NULL,"null"},
	{(unsigned int)SQL_CB_NON_NULL,"non_null"},
	{0,NULL},
};

Getinfo_values getinfo_values_convert_bigint[] = {
	{(unsigned int)SQL_CVT_BIGINT,"bigint"},
	{(unsigned int)SQL_CVT_BINARY,"binary"},
	{(unsigned int)SQL_CVT_BIT,"bit"},
	{(unsigned int)SQL_CVT_CHAR,"char"},
	{(unsigned int)SQL_CVT_DATE,"date"},
	{(unsigned int)SQL_CVT_DECIMAL,"decimal"},
	{(unsigned int)SQL_CVT_DOUBLE,"double"},
	{(unsigned int)SQL_CVT_FLOAT,"float"},
	{(unsigned int)SQL_CVT_INTEGER,"integer"},
	{(unsigned int)SQL_CVT_INTERVAL_YEAR_MONTH,"interval_year_month"},
	{(unsigned int)SQL_CVT_INTERVAL_DAY_TIME,"interval_day_time"},
	{(unsigned int)SQL_CVT_LONGVARBINARY,"longvarbinary"},
	{(unsigned int)SQL_CVT_LONGVARCHAR,"longvarchar"},
	{(unsigned int)SQL_CVT_NUMERIC,"numeric"},
	{(unsigned int)SQL_CVT_REAL,"real"},
	{(unsigned int)SQL_CVT_SMALLINT,"smallint"},
	{(unsigned int)SQL_CVT_TIME,"time"},
	{(unsigned int)SQL_CVT_TIMESTAMP,"timestamp"},
	{(unsigned int)SQL_CVT_TINYINT,"tinyint"},
	{(unsigned int)SQL_CVT_VARBINARY,"varbinary"},
	{(unsigned int)SQL_CVT_VARCHAR,"varchar"},
	{0,NULL},
};

Getinfo_values getinfo_values_convert_functions[] = {
	{(unsigned int)SQL_FN_CVT_CAST,"cast"},
	{(unsigned int)SQL_FN_CVT_CONVERT,"convert"},
	{0,NULL},
};

Getinfo_values getinfo_values_correlation_name[] = {
	{(unsigned int)SQL_CN_NONE,"none"},
	{(unsigned int)SQL_CN_DIFFERENT,"different"},
	{(unsigned int)SQL_CN_ANY,"any"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_assertion[] = {
	{(unsigned int)SQL_CA_CREATE_ASSERTION,"create_assertion"},
	{(unsigned int)SQL_CA_CONSTRAINT_INITIALLY_DEFERRED,"constraint_initially_deferred"},
	{(unsigned int)SQL_CA_CONSTRAINT_INITIALLY_IMMEDIATE,"constraint_initially_immediate"},
	{(unsigned int)SQL_CA_CONSTRAINT_DEFERRABLE,"constraint_deferrable"},
	{(unsigned int)SQL_CA_CONSTRAINT_NON_DEFERRABLE,"constraint_non_deferrable"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_character_set[] = {
	{(unsigned int)SQL_CCS_CREATE_CHARACTER_SET,"create_character_set"},
	{(unsigned int)SQL_CCS_COLLATE_CLAUSE,"collate_clause"},
	{(unsigned int)SQL_CCS_LIMITED_COLLATION,"limited_collation"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_collation[] = {
	{(unsigned int)SQL_CCOL_CREATE_COLLATION,"collation"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_domain[] = {
	{(unsigned int)SQL_CDO_CREATE_DOMAIN,"create_domain"},
	{(unsigned int)SQL_CDO_CONSTRAINT_NAME_DEFINITION,"constraint_name_definition"},
	{(unsigned int)SQL_CDO_DEFAULT,"default"},
	{(unsigned int)SQL_CDO_CONSTRAINT,"constraint"},
	{(unsigned int)SQL_CDO_COLLATION,"collation"},
	{(unsigned int)SQL_CDO_CONSTRAINT_INITIALLY_DEFERRED,"constraint_initially_deferred"},
	{(unsigned int)SQL_CDO_CONSTRAINT_INITIALLY_IMMEDIATE,"constraint_initially_immediate"},
	{(unsigned int)SQL_CDO_CONSTRAINT_DEFERRABLE,"constraint_deferrable"},
	{(unsigned int)SQL_CDO_CONSTRAINT_NON_DEFERRABLE,"constraint_non_deferrable"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_schema[] = {
	{(unsigned int)SQL_CS_CREATE_SCHEMA,"create_schema"},
	{(unsigned int)SQL_CS_AUTHORIZATION,"authorization"},
	{(unsigned int)SQL_CS_DEFAULT_CHARACTER_SET,"default_character_set"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_table[] = {
	{(unsigned int)SQL_CT_CREATE_TABLE,"create_table"},
	{(unsigned int)SQL_CT_TABLE_CONSTRAINT,"table_constraint"},
	{(unsigned int)SQL_CT_CONSTRAINT_NAME_DEFINITION,"constraint_name_definition"},
	{(unsigned int)SQL_CT_COMMIT_PRESERVE,"commit_preserve"},
	{(unsigned int)SQL_CT_COMMIT_DELETE,"commit_delete"},
	{(unsigned int)SQL_CT_GLOBAL_TEMPORARY,"global_temporary"},
	{(unsigned int)SQL_CT_LOCAL_TEMPORARY,"local_temporary"},
	{(unsigned int)SQL_CT_COLUMN_CONSTRAINT,"column_constraint"},
	{(unsigned int)SQL_CT_COLUMN_DEFAULT,"column_default"},
	{(unsigned int)SQL_CT_COLUMN_COLLATION,"column_collation"},
	{(unsigned int)SQL_CT_CONSTRAINT_INITIALLY_DEFERRED,"constraint_initially_deferred"},
	{(unsigned int)SQL_CT_CONSTRAINT_INITIALLY_IMMEDIATE,"constraint_initially_immediate"},
	{(unsigned int)SQL_CT_CONSTRAINT_DEFERRABLE,"constraint_deferrable"},
	{(unsigned int)SQL_CT_CONSTRAINT_NON_DEFERRABLE,"constraint_non_deferrable"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_translation[] = {
	{(unsigned int)SQL_CTR_CREATE_TRANSLATION,"translation"},
	{0,NULL},
};

Getinfo_values getinfo_values_create_view[] = {
	{(unsigned int)SQL_CV_CREATE_VIEW,"create_view"},
	{(unsigned int)SQL_CV_CHECK_OPTION,"check_option"},
	{(unsigned int)SQL_CV_CASCADED,"cascaded"},
	{(unsigned int)SQL_CV_LOCAL,"local"},
	{0,NULL},
};

Getinfo_values getinfo_values_cursor_commit_behavior[] = {
	{(unsigned int)SQL_CB_DELETE,"delete"},
	{(unsigned int)SQL_CB_CLOSE,"close"},
	{(unsigned int)SQL_CB_PRESERVE,"preserve"},
	{0,NULL},
};

Getinfo_values getinfo_values_cursor_rollback_behavior[] = {
	{(unsigned int)SQL_CB_DELETE,"delete"},
	{(unsigned int)SQL_CB_CLOSE,"close"},
	{(unsigned int)SQL_CB_PRESERVE,"preserve"},
	{0,NULL},
};

Getinfo_values getinfo_values_cursor_rollback_sql_cursor_sensitivity[] = {
	{(unsigned int)SQL_INSENSITIVE,"insensitive"},
	{(unsigned int)SQL_UNSPECIFIED,"unspecified"},
	{(unsigned int)SQL_SENSITIVE,"sensitive"},
	{0,NULL},
};

Getinfo_values getinfo_values_datetime_literals[] = {
	{(unsigned int)SQL_DL_SQL92_DATE,"date"},
	{(unsigned int)SQL_DL_SQL92_TIME,"time"},
	{(unsigned int)SQL_DL_SQL92_TIMESTAMP,"timestamp"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_YEAR,"interval_year"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_MONTH,"interval_month"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_DAY,"interval_day"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_HOUR,"interval_hour"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_MINUTE,"interval_minute"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_SECOND,"interval_second"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_YEAR_TO_MONTH,"interval_year_to_month"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_DAY_TO_HOUR,"interval_day_to_hour"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_DAY_TO_MINUTE,"interval_day_to_minute"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_DAY_TO_SECOND,"interval_day_to_second"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_HOUR_TO_MINUTE,"interval_hour_to_minute"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_HOUR_TO_SECOND,"interval_hour_to_second"},
	{(unsigned int)SQL_DL_SQL92_INTERVAL_MINUTE_TO_SECOND,"interval_minute_to_second"},
	{0,NULL},
};

Getinfo_values getinfo_values_ddl_index[] = {
	{(unsigned int)SQL_DI_CREATE_INDEX,"create_index"},
	{(unsigned int)SQL_DI_DROP_INDEX,"drop_index"},
	{0,NULL},
};

Getinfo_values getinfo_values_default_txn_isolation[] = {
	{(unsigned int)SQL_TXN_READ_UNCOMMITTED,"read_uncommitted"},
	{(unsigned int)SQL_TXN_READ_COMMITTED,"read_committed"},
	{(unsigned int)SQL_TXN_REPEATABLE_READ,"repeatable_read"},
	{(unsigned int)SQL_TXN_SERIALIZABLE,"serializable"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_assertion[] = {
	{(unsigned int)SQL_DA_DROP_ASSERTION,"assertion"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_character_set[] = {
	{(unsigned int)SQL_DCS_DROP_CHARACTER_SET,"set"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_collation[] = {
	{(unsigned int)SQL_DC_DROP_COLLATION,"collation"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_domain[] = {
	{(unsigned int)SQL_DD_DROP_DOMAIN,"drop_domain"},
	{(unsigned int)SQL_DD_CASCADE,"cascade"},
	{(unsigned int)SQL_DD_RESTRICT,"restrict"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_schema[] = {
	{(unsigned int)SQL_DS_DROP_SCHEMA,"drop_schema"},
	{(unsigned int)SQL_DS_CASCADE,"cascade"},
	{(unsigned int)SQL_DS_RESTRICT,"restrict"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_table[] = {
	{(unsigned int)SQL_DT_DROP_TABLE,"drop_table"},
	{(unsigned int)SQL_DT_CASCADE,"cascade"},
	{(unsigned int)SQL_DT_RESTRICT,"restrict"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_translation[] = {
	{(unsigned int)SQL_DTR_DROP_TRANSLATION,"translation"},
	{0,NULL},
};

Getinfo_values getinfo_values_drop_view[] = {
	{(unsigned int)SQL_DV_DROP_VIEW,"drop_view"},
	{(unsigned int)SQL_DV_CASCADE,"cascade"},
	{(unsigned int)SQL_DV_RESTRICT,"restrict"},
	{0,NULL},
};

Getinfo_values getinfo_values_dynamic_cursor_attributes1[] = {
	{(unsigned int)SQL_CA1_NEXT,"next"},
	{(unsigned int)SQL_CA1_ABSOLUTE,"absolute"},
	{(unsigned int)SQL_CA1_RELATIVE,"relative"},
	{(unsigned int)SQL_CA1_BOOKMARK,"bookmark"},
	{(unsigned int)SQL_CA1_LOCK_EXCLUSIVE,"lock_exclusive"},
	{(unsigned int)SQL_CA1_LOCK_NO_CHANGE,"lock_no_change"},
	{(unsigned int)SQL_CA1_LOCK_UNLOCK,"lock_unlock"},
	{(unsigned int)SQL_CA1_POS_POSITION,"pos_position"},
	{(unsigned int)SQL_CA1_POS_UPDATE,"pos_update"},
	{(unsigned int)SQL_CA1_POS_DELETE,"pos_delete"},
	{(unsigned int)SQL_CA1_POS_REFRESH,"pos_refresh"},
	{(unsigned int)SQL_CA1_POSITIONED_UPDATE,"positioned_update"},
	{(unsigned int)SQL_CA1_POSITIONED_DELETE,"positioned_delete"},
	{(unsigned int)SQL_CA1_SELECT_FOR_UPDATE,"select_for_update"},
	{(unsigned int)SQL_CA1_BULK_ADD,"bulk_add"},
	{(unsigned int)SQL_CA1_BULK_UPDATE_BY_BOOKMARK,"bulk_update_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_DELETE_BY_BOOKMARK,"bulk_delete_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_FETCH_BY_BOOKMARK,"bulk_fetch_by_bookmark"},
	{0,NULL},
};

Getinfo_values getinfo_values_dynamic_cursor_attributes2[] = {
	{(unsigned int)SQL_CA2_READ_ONLY_CONCURRENCY,"read_only_concurrency"},
	{(unsigned int)SQL_CA2_LOCK_CONCURRENCY,"lock_concurrency"},
	{(unsigned int)SQL_CA2_OPT_ROWVER_CONCURRENCY,"opt_rowver_concurrency"},
	{(unsigned int)SQL_CA2_OPT_VALUES_CONCURRENCY,"opt_values_concurrency"},
	{(unsigned int)SQL_CA2_SENSITIVITY_ADDITIONS,"sensitivity_additions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_DELETIONS,"sensitivity_deletions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_UPDATES,"sensitivity_updates"},
	{(unsigned int)SQL_CA2_MAX_ROWS_SELECT,"max_rows_select"},
	{(unsigned int)SQL_CA2_MAX_ROWS_INSERT,"max_rows_insert"},
	{(unsigned int)SQL_CA2_MAX_ROWS_DELETE,"max_rows_delete"},
	{(unsigned int)SQL_CA2_MAX_ROWS_UPDATE,"max_rows_update"},
	{(unsigned int)SQL_CA2_MAX_ROWS_CATALOG,"max_rows_catalog"},
	{(unsigned int)SQL_CA2_MAX_ROWS_AFFECTS_ALL,"max_rows_affects_all"},
	{(unsigned int)SQL_CA2_CRC_EXACT,"crc_exact"},
	{(unsigned int)SQL_CA2_CRC_APPROXIMATE,"crc_approximate"},
	{(unsigned int)SQL_CA2_SIMULATE_NON_UNIQUE,"simulate_non_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_TRY_UNIQUE,"simulate_try_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_UNIQUE,"simulate_unique"},
	{0,NULL},
};

Getinfo_values getinfo_values_file_usage[] = {
	{(unsigned int)SQL_FILE_NOT_SUPPORTED,"not_supported"},
	{(unsigned int)SQL_FILE_TABLE,"table"},
	{(unsigned int)SQL_FILE_CATALOG,"catalog"},
	{0,NULL},
};

Getinfo_values getinfo_values_forward_only_cursor_attributes1[] = {
	{(unsigned int)SQL_CA1_NEXT,"next"},
	{(unsigned int)SQL_CA1_LOCK_EXCLUSIVE,"lock_exclusive"},
	{(unsigned int)SQL_CA1_LOCK_NO_CHANGE,"lock_no_change"},
	{(unsigned int)SQL_CA1_LOCK_UNLOCK,"lock_unlock"},
	{(unsigned int)SQL_CA1_POS_POSITION,"pos_position"},
	{(unsigned int)SQL_CA1_POS_UPDATE,"pos_update"},
	{(unsigned int)SQL_CA1_POS_DELETE,"pos_delete"},
	{(unsigned int)SQL_CA1_POS_REFRESH,"pos_refresh"},
	{(unsigned int)SQL_CA1_POSITIONED_UPDATE,"positioned_update"},
	{(unsigned int)SQL_CA1_POSITIONED_DELETE,"positioned_delete"},
	{(unsigned int)SQL_CA1_SELECT_FOR_UPDATE,"select_for_update"},
	{(unsigned int)SQL_CA1_BULK_ADD,"bulk_add"},
	{(unsigned int)SQL_CA1_BULK_UPDATE_BY_BOOKMARK,"bulk_update_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_DELETE_BY_BOOKMARK,"bulk_delete_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_FETCH_BY_BOOKMARK,"bulk_fetch_by_bookmark"},
	{0,NULL},
};

Getinfo_values getinfo_values_forward_only_cursor_attributes2[] = {
	{(unsigned int)SQL_CA2_READ_ONLY_CONCURRENCY,"read_only_concurrency"},
	{(unsigned int)SQL_CA2_LOCK_CONCURRENCY,"lock_concurrency"},
	{(unsigned int)SQL_CA2_OPT_ROWVER_CONCURRENCY,"opt_rowver_concurrency"},
	{(unsigned int)SQL_CA2_OPT_VALUES_CONCURRENCY,"opt_values_concurrency"},
	{(unsigned int)SQL_CA2_SENSITIVITY_ADDITIONS,"sensitivity_additions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_DELETIONS,"sensitivity_deletions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_UPDATES,"sensitivity_updates"},
	{(unsigned int)SQL_CA2_MAX_ROWS_SELECT,"max_rows_select"},
	{(unsigned int)SQL_CA2_MAX_ROWS_INSERT,"max_rows_insert"},
	{(unsigned int)SQL_CA2_MAX_ROWS_DELETE,"max_rows_delete"},
	{(unsigned int)SQL_CA2_MAX_ROWS_UPDATE,"max_rows_update"},
	{(unsigned int)SQL_CA2_MAX_ROWS_CATALOG,"max_rows_catalog"},
	{(unsigned int)SQL_CA2_MAX_ROWS_AFFECTS_ALL,"max_rows_affects_all"},
	{(unsigned int)SQL_CA2_CRC_EXACT,"crc_exact"},
	{(unsigned int)SQL_CA2_CRC_APPROXIMATE,"crc_approximate"},
	{(unsigned int)SQL_CA2_SIMULATE_NON_UNIQUE,"simulate_non_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_TRY_UNIQUE,"simulate_try_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_UNIQUE,"simulate_unique"},
	{0,NULL},
};

Getinfo_values getinfo_values_getdata_extensions[] = {
	{(unsigned int)SQL_GD_ANY_COLUMN,"any_column"},
	{(unsigned int)SQL_GD_ANY_ORDER,"any_order"},
	{(unsigned int)SQL_GD_BLOCK,"block"},
	{(unsigned int)SQL_GD_BOUND,"bound"},
	{0,NULL},
};

Getinfo_values getinfo_values_group_by[] = {
	{(unsigned int)SQL_GB_COLLATE,"collate"},
	{(unsigned int)SQL_GB_NOT_SUPPORTED,"not_supported"},
	{(unsigned int)SQL_GB_GROUP_BY_EQUALS_SELECT,"group_by_equals_select"},
	{(unsigned int)SQL_GB_GROUP_BY_CONTAINS_SELECT,"group_by_contains_select"},
	{(unsigned int)SQL_GB_NO_RELATION,"no_relation"},
	{0,NULL},
};

Getinfo_values getinfo_values_identifier_case[] = {
	{(unsigned int)SQL_IC_UPPER,"upper"},
	{(unsigned int)SQL_IC_LOWER,"lower"},
	{(unsigned int)SQL_IC_SENSITIVE,"sensitive"},
	{(unsigned int)SQL_IC_MIXED,"mixed"},
	{0,NULL},
};

Getinfo_values getinfo_values_identifier_quote_char[] = {
	{(unsigned int)SQL_IK_NONE,"none"},
	{(unsigned int)SQL_IK_ASC,"asc"},
	{(unsigned int)SQL_IK_DESC,"desc"},
	{(unsigned int)SQL_IK_ALL,"all"},
	{0,NULL},
};

Getinfo_values getinfo_values_info_schema_views[] = {
	{(unsigned int)SQL_ISV_ASSERTIONS,"assertions"},
	{(unsigned int)SQL_ISV_CHARACTER_SETS,"character_sets"},
	{(unsigned int)SQL_ISV_CHECK_CONSTRAINTS,"check_constraints"},
	{(unsigned int)SQL_ISV_COLLATIONS,"collations"},
	{(unsigned int)SQL_ISV_COLUMN_DOMAIN_USAGE,"column_domain_usage"},
	{(unsigned int)SQL_ISV_COLUMN_PRIVILEGES,"column_privileges"},
	{(unsigned int)SQL_ISV_COLUMNS,"columns"},
	{(unsigned int)SQL_ISV_CONSTRAINT_COLUMN_USAGE,"constraint_column_usage"},
	{(unsigned int)SQL_ISV_CONSTRAINT_TABLE_USAGE,"constraint_table_usage"},
	{(unsigned int)SQL_ISV_DOMAIN_CONSTRAINTS,"domain_constraints"},
	{(unsigned int)SQL_ISV_DOMAINS,"domains"},
	{(unsigned int)SQL_ISV_KEY_COLUMN_USAGE,"key_column_usage"},
	{(unsigned int)SQL_ISV_REFERENTIAL_CONSTRAINTS,"referential_constraints"},
	{(unsigned int)SQL_ISV_SCHEMATA,"schemata"},
	{(unsigned int)SQL_ISV_SQL_LANGUAGES,"sql_languages"},
	{(unsigned int)SQL_ISV_TABLE_CONSTRAINTS,"table_constraints"},
	{(unsigned int)SQL_ISV_TABLE_PRIVILEGES,"table_privileges"},
	{(unsigned int)SQL_ISV_TABLES,"tables"},
	{(unsigned int)SQL_ISV_TRANSLATIONS,"translations"},
	{(unsigned int)SQL_ISV_USAGE_PRIVILEGES,"usage_privileges"},
	{(unsigned int)SQL_ISV_VIEW_COLUMN_USAGE,"view_column_usage"},
	{(unsigned int)SQL_ISV_VIEW_TABLE_USAGE,"view_table_usage"},
	{(unsigned int)SQL_ISV_VIEWS,"views"},
	{0,NULL},
};

Getinfo_values getinfo_values_insert_statement[] = {
	{(unsigned int)SQL_IS_INSERT_LITERALS,"insert_literals"},
	{(unsigned int)SQL_IS_INSERT_SEARCHED,"insert_searched"},
	{(unsigned int)SQL_IS_SELECT_INTO,"select_into"},
	{0,NULL},
};

Getinfo_values getinfo_values_keyset_cursor_attributes1[] = {
	{(unsigned int)SQL_CA1_NEXT,"next"},
	{(unsigned int)SQL_CA1_ABSOLUTE,"absolute"},
	{(unsigned int)SQL_CA1_RELATIVE,"relative"},
	{(unsigned int)SQL_CA1_BOOKMARK,"bookmark"},
	{(unsigned int)SQL_CA1_LOCK_EXCLUSIVE,"lock_exclusive"},
	{(unsigned int)SQL_CA1_LOCK_NO_CHANGE,"lock_no_change"},
	{(unsigned int)SQL_CA1_LOCK_UNLOCK,"lock_unlock"},
	{(unsigned int)SQL_CA1_POS_POSITION,"pos_position"},
	{(unsigned int)SQL_CA1_POS_UPDATE,"pos_update"},
	{(unsigned int)SQL_CA1_POS_DELETE,"pos_delete"},
	{(unsigned int)SQL_CA1_POS_REFRESH,"pos_refresh"},
	{(unsigned int)SQL_CA1_POSITIONED_UPDATE,"positioned_update"},
	{(unsigned int)SQL_CA1_POSITIONED_DELETE,"positioned_delete"},
	{(unsigned int)SQL_CA1_SELECT_FOR_UPDATE,"select_for_update"},
	{(unsigned int)SQL_CA1_BULK_ADD,"bulk_add"},
	{(unsigned int)SQL_CA1_BULK_UPDATE_BY_BOOKMARK,"bulk_update_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_DELETE_BY_BOOKMARK,"bulk_delete_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_FETCH_BY_BOOKMARK,"bulk_fetch_by_bookmark"},
	{0,NULL},
};

Getinfo_values getinfo_values_keyset_cursor_attributes2[] = {
	{(unsigned int)SQL_CA2_READ_ONLY_CONCURRENCY,"read_only_concurrency"},
	{(unsigned int)SQL_CA2_LOCK_CONCURRENCY,"lock_concurrency"},
	{(unsigned int)SQL_CA2_OPT_ROWVER_CONCURRENCY,"opt_rowver_concurrency"},
	{(unsigned int)SQL_CA2_OPT_VALUES_CONCURRENCY,"opt_values_concurrency"},
	{(unsigned int)SQL_CA2_SENSITIVITY_ADDITIONS,"sensitivity_additions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_DELETIONS,"sensitivity_deletions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_UPDATES,"sensitivity_updates"},
	{(unsigned int)SQL_CA2_MAX_ROWS_SELECT,"max_rows_select"},
	{(unsigned int)SQL_CA2_MAX_ROWS_INSERT,"max_rows_insert"},
	{(unsigned int)SQL_CA2_MAX_ROWS_DELETE,"max_rows_delete"},
	{(unsigned int)SQL_CA2_MAX_ROWS_UPDATE,"max_rows_update"},
	{(unsigned int)SQL_CA2_MAX_ROWS_CATALOG,"max_rows_catalog"},
	{(unsigned int)SQL_CA2_MAX_ROWS_AFFECTS_ALL,"max_rows_affects_all"},
	{(unsigned int)SQL_CA2_CRC_EXACT,"crc_exact"},
	{(unsigned int)SQL_CA2_CRC_APPROXIMATE,"crc_approximate"},
	{(unsigned int)SQL_CA2_SIMULATE_NON_UNIQUE,"simulate_non_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_TRY_UNIQUE,"simulate_try_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_UNIQUE,"simulate_unique"},
	{0,NULL},
};

Getinfo_values getinfo_values_non_nullable_columns[] = {
	{(unsigned int)SQL_NNC_NULL,"null"},
	{(unsigned int)SQL_NNC_NON_NULL,"non_null"},
	{0,NULL},
};

Getinfo_values getinfo_values_null_collation[] = {
	{(unsigned int)SQL_NC_END,"end"},
	{(unsigned int)SQL_NC_HIGH,"high"},
	{(unsigned int)SQL_NC_LOW,"low"},
	{(unsigned int)SQL_NC_START,"start"},
	{0,NULL},
};

Getinfo_values getinfo_values_numeric_functions[] = {
	{(unsigned int)SQL_FN_NUM_ABS,"abs"},
	{(unsigned int)SQL_FN_NUM_ACOS,"acos"},
	{(unsigned int)SQL_FN_NUM_ASIN,"asin"},
	{(unsigned int)SQL_FN_NUM_ATAN,"atan"},
	{(unsigned int)SQL_FN_NUM_ATAN2,"atan2"},
	{(unsigned int)SQL_FN_NUM_CEILING,"ceiling"},
	{(unsigned int)SQL_FN_NUM_COS,"cos"},
	{(unsigned int)SQL_FN_NUM_COT,"cot"},
	{(unsigned int)SQL_FN_NUM_DEGREES,"degrees"},
	{(unsigned int)SQL_FN_NUM_EXP,"exp"},
	{(unsigned int)SQL_FN_NUM_FLOOR,"floor"},
	{(unsigned int)SQL_FN_NUM_LOG,"log"},
	{(unsigned int)SQL_FN_NUM_LOG10,"log10"},
	{(unsigned int)SQL_FN_NUM_MOD,"mod"},
	{(unsigned int)SQL_FN_NUM_PI,"pi"},
	{(unsigned int)SQL_FN_NUM_POWER,"power"},
	{(unsigned int)SQL_FN_NUM_RADIANS,"radians"},
	{(unsigned int)SQL_FN_NUM_RAND,"rand"},
	{(unsigned int)SQL_FN_NUM_ROUND,"round"},
	{(unsigned int)SQL_FN_NUM_SIGN,"sign"},
	{(unsigned int)SQL_FN_NUM_SIN,"sin"},
	{(unsigned int)SQL_FN_NUM_SQRT,"sqrt"},
	{(unsigned int)SQL_FN_NUM_TAN,"tan"},
	{(unsigned int)SQL_FN_NUM_TRUNCATE,"truncate"},
	{0,NULL},
};

Getinfo_values getinfo_values_odbc_interface_conformance[] = {
	{(unsigned int)SQL_OIC_CORE,"core"},
	{(unsigned int)SQL_OIC_LEVEL1,"level1"},
	{(unsigned int)SQL_OIC_LEVEL2,"level2"},
	{0,NULL},
};

Getinfo_values getinfo_values_oj_capabilities[] = {
	{(unsigned int)SQL_OJ_LEFT,"left"},
	{(unsigned int)SQL_OJ_RIGHT,"right"},
	{(unsigned int)SQL_OJ_FULL,"full"},
	{(unsigned int)SQL_OJ_NESTED,"nested"},
	{(unsigned int)SQL_OJ_NOT_ORDERED,"not_ordered"},
	{(unsigned int)SQL_OJ_INNER,"inner"},
	{(unsigned int)SQL_OJ_ALL_COMPARISON_OPS,"all_comparison_ops"},
	{0,NULL},
};

Getinfo_values getinfo_values_param_array_row_counts[] = {
	{(unsigned int)SQL_PARC_BATCH,"batch"},
	{(unsigned int)SQL_PARC_NO_BATCH,"no_batch"},
	{0,NULL},
};

Getinfo_values getinfo_values_param_array_selects[] = {
	{(unsigned int)SQL_PAS_BATCH,"batch"},
	{(unsigned int)SQL_PAS_NO_BATCH,"no_batch"},
	{(unsigned int)SQL_PAS_NO_SELECT,"no_select"},
	{0,NULL},
};

Getinfo_values getinfo_values_pos_operations[] = {
	{(unsigned int)SQL_POS_POSITION,"position"},
	{(unsigned int)SQL_POS_REFRESH,"refresh"},
	{(unsigned int)SQL_POS_UPDATE,"update"},
	{(unsigned int)SQL_POS_DELETE,"delete"},
	{(unsigned int)SQL_POS_ADD,"add"},
	{0,NULL},
};

Getinfo_values getinfo_values_quoted_identifier_case[] = {
	{(unsigned int)SQL_IC_UPPER,"upper"},
	{(unsigned int)SQL_IC_LOWER,"lower"},
	{(unsigned int)SQL_IC_SENSITIVE,"sensitive"},
	{(unsigned int)SQL_IC_MIXED,"mixed"},
	{0,NULL},
};

Getinfo_values getinfo_values_schema_usage[] = {
	{(unsigned int)SQL_SU_DML_STATEMENTS,"dml_statements"},
	{(unsigned int)SQL_SU_PROCEDURE_INVOCATION,"procedure_invocation"},
	{(unsigned int)SQL_SU_TABLE_DEFINITION,"table_definition"},
	{(unsigned int)SQL_SU_INDEX_DEFINITION,"index_definition"},
	{(unsigned int)SQL_SU_PRIVILEGE_DEFINITION,"privilege_definition"},
	{0,NULL},
};

Getinfo_values getinfo_values_scroll_options[] = {
	{(unsigned int)SQL_SO_FORWARD_ONLY,"forward_only"},
	{(unsigned int)SQL_SO_STATIC,"static"},
	{(unsigned int)SQL_SO_KEYSET_DRIVEN,"keyset_driven"},
	{(unsigned int)SQL_SO_DYNAMIC,"dynamic"},
	{(unsigned int)SQL_SO_MIXED,"mixed"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql_conformance[] = {
	{(unsigned int)SQL_SC_SQL92_ENTRY,"sql92_entry"},
	{(unsigned int)SQL_SC_FIPS127_2_TRANSITIONAL,"fips127_2_transitional"},
	{(unsigned int)SQL_SC_SQL92_FULL,"sql92_full"},
	{(unsigned int)SQL_SC_SQL92_INTERMEDIATE,"sql92_intermediate"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_datetime_functions[] = {
	{(unsigned int)SQL_SDF_CURRENT_DATE,"date"},
	{(unsigned int)SQL_SDF_CURRENT_TIME,"time"},
	{(unsigned int)SQL_SDF_CURRENT_TIMESTAMP,"timestamp"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_foreign_key_delete_rule[] = {
	{(unsigned int)SQL_SFKD_CASCADE,"cascade"},
	{(unsigned int)SQL_SFKD_NO_ACTION,"no_action"},
	{(unsigned int)SQL_SFKD_SET_DEFAULT,"set_default"},
	{(unsigned int)SQL_SFKD_SET_NULL,"set_null"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_foreign_key_update_rule[] = {
	{(unsigned int)SQL_SFKU_CASCADE,"cascade"},
	{(unsigned int)SQL_SFKU_NO_ACTION,"no_action"},
	{(unsigned int)SQL_SFKU_SET_DEFAULT,"set_default"},
	{(unsigned int)SQL_SFKU_SET_NULL,"set_null"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_grant[] = {
	{(unsigned int)SQL_SG_DELETE_TABLE,"delete_table"},
	{(unsigned int)SQL_SG_INSERT_COLUMN,"insert_column"},
	{(unsigned int)SQL_SG_INSERT_TABLE,"insert_table"},
	{(unsigned int)SQL_SG_REFERENCES_TABLE,"references_table"},
	{(unsigned int)SQL_SG_REFERENCES_COLUMN,"references_column"},
	{(unsigned int)SQL_SG_SELECT_TABLE,"select_table"},
	{(unsigned int)SQL_SG_UPDATE_COLUMN,"update_column"},
	{(unsigned int)SQL_SG_UPDATE_TABLE,"update_table"},
	{(unsigned int)SQL_SG_USAGE_ON_DOMAIN,"usage_on_domain"},
	{(unsigned int)SQL_SG_USAGE_ON_CHARACTER_SET,"usage_on_character_set"},
	{(unsigned int)SQL_SG_USAGE_ON_COLLATION,"usage_on_collation"},
	{(unsigned int)SQL_SG_USAGE_ON_TRANSLATION,"usage_on_translation"},
	{(unsigned int)SQL_SG_WITH_GRANT_OPTION,"with_grant_option"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_numeric_value_functions[] = {
	{(unsigned int)SQL_SNVF_BIT_LENGTH,"bit_length"},
	{(unsigned int)SQL_SNVF_CHAR_LENGTH,"char_length"},
	{(unsigned int)SQL_SNVF_CHARACTER_LENGTH,"character_length"},
	{(unsigned int)SQL_SNVF_EXTRACT,"extract"},
	{(unsigned int)SQL_SNVF_OCTET_LENGTH,"octet_length"},
	{(unsigned int)SQL_SNVF_POSITION,"position"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_predicates[] = {
	{(unsigned int)SQL_SP_BETWEEN,"between"},
	{(unsigned int)SQL_SP_COMPARISON,"comparison"},
	{(unsigned int)SQL_SP_EXISTS,"exists"},
	{(unsigned int)SQL_SP_IN,"in"},
	{(unsigned int)SQL_SP_ISNOTNULL,"isnotnull"},
	{(unsigned int)SQL_SP_ISNULL,"isnull"},
	{(unsigned int)SQL_SP_LIKE,"like"},
	{(unsigned int)SQL_SP_MATCH_FULL,"match_full"},
	{(unsigned int)SQL_SP_MATCH_PARTIAL,"match_partial"},
	{(unsigned int)SQL_SP_MATCH_UNIQUE_FULL,"match_unique_full"},
	{(unsigned int)SQL_SP_MATCH_UNIQUE_PARTIAL,"match_unique_partial"},
	{(unsigned int)SQL_SP_OVERLAPS,"overlaps"},
	{(unsigned int)SQL_SP_QUANTIFIED_COMPARISON,"quantified_comparison"},
	{(unsigned int)SQL_SP_UNIQUE,"unique"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_relational_join_operators[] = {
	{(unsigned int)SQL_SRJO_CORRESPONDING_CLAUSE,"corresponding_clause"},
	{(unsigned int)SQL_SRJO_CROSS_JOIN,"cross_join"},
	{(unsigned int)SQL_SRJO_EXCEPT_JOIN,"except_join"},
	{(unsigned int)SQL_SRJO_FULL_OUTER_JOIN,"full_outer_join"},
	{(unsigned int)SQL_SRJO_INNER_JOIN,"inner_join"},
	{(unsigned int)SQL_SRJO_INTERSECT_JOIN,"intersect_join"},
	{(unsigned int)SQL_SRJO_LEFT_OUTER_JOIN,"left_outer_join"},
	{(unsigned int)SQL_SRJO_NATURAL_JOIN,"natural_join"},
	{(unsigned int)SQL_SRJO_RIGHT_OUTER_JOIN,"right_outer_join"},
	{(unsigned int)SQL_SRJO_UNION_JOIN,"union_join"},
	{(unsigned int)SQL_SRJO_INNER_JOIN,"inner_join"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_revoke[] = {
	{(unsigned int)SQL_SR_CASCADE,"cascade"},
	{(unsigned int)SQL_SR_DELETE_TABLE,"delete_table"},
	{(unsigned int)SQL_SR_GRANT_OPTION_FOR,"grant_option_for"},
	{(unsigned int)SQL_SR_INSERT_COLUMN,"insert_column"},
	{(unsigned int)SQL_SR_INSERT_TABLE,"insert_table"},
	{(unsigned int)SQL_SR_REFERENCES_COLUMN,"references_column"},
	{(unsigned int)SQL_SR_REFERENCES_TABLE,"references_table"},
	{(unsigned int)SQL_SR_RESTRICT,"restrict"},
	{(unsigned int)SQL_SR_SELECT_TABLE,"select_table"},
	{(unsigned int)SQL_SR_UPDATE_COLUMN,"update_column"},
	{(unsigned int)SQL_SR_UPDATE_TABLE,"update_table"},
	{(unsigned int)SQL_SR_USAGE_ON_DOMAIN,"usage_on_domain"},
	{(unsigned int)SQL_SR_USAGE_ON_CHARACTER_SET,"usage_on_character_set"},
	{(unsigned int)SQL_SR_USAGE_ON_COLLATION,"usage_on_collation"},
	{(unsigned int)SQL_SR_USAGE_ON_TRANSLATION,"usage_on_translation"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_row_value_constructor[] = {
	{(unsigned int)SQL_SRVC_VALUE_EXPRESSION,"value_expression"},
	{(unsigned int)SQL_SRVC_NULL,"null"},
	{(unsigned int)SQL_SRVC_DEFAULT,"default"},
	{(unsigned int)SQL_SRVC_ROW_SUBQUERY,"row_subquery"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_string_functions[] = {
	{(unsigned int)SQL_SSF_CONVERT,"convert"},
	{(unsigned int)SQL_SSF_LOWER,"lower"},
	{(unsigned int)SQL_SSF_UPPER,"upper"},
	{(unsigned int)SQL_SSF_SUBSTRING,"substring"},
	{(unsigned int)SQL_SSF_TRANSLATE,"translate"},
	{(unsigned int)SQL_SSF_TRIM_BOTH,"trim_both"},
	{(unsigned int)SQL_SSF_TRIM_LEADING,"trim_leading"},
	{(unsigned int)SQL_SSF_TRIM_TRAILING,"trim_trailing"},
	{0,NULL},
};

Getinfo_values getinfo_values_sql92_value_expressions[] = {
	{(unsigned int)SQL_SVE_CASE,"case"},
	{(unsigned int)SQL_SVE_CAST,"cast"},
	{(unsigned int)SQL_SVE_COALESCE,"coalesce"},
	{(unsigned int)SQL_SVE_NULLIF,"nullif"},
	{0,NULL},
};

Getinfo_values getinfo_values_standard_cli_conformance[] = {
	{(unsigned int)SQL_SCC_XOPEN_CLI_VERSION1,"xopen_cli_version1"},
	{(unsigned int)SQL_SCC_ISO92_CLI,"iso92_cli"},
	{0,NULL},
};

Getinfo_values getinfo_values_static_cursor_attributes1[] = {
	{(unsigned int)SQL_CA1_NEXT,"next"},
	{(unsigned int)SQL_CA1_ABSOLUTE,"absolute"},
	{(unsigned int)SQL_CA1_RELATIVE,"relative"},
	{(unsigned int)SQL_CA1_BOOKMARK,"bookmark"},
	{(unsigned int)SQL_CA1_LOCK_NO_CHANGE,"lock_no_change"},
	{(unsigned int)SQL_CA1_LOCK_EXCLUSIVE,"lock_exclusive"},
	{(unsigned int)SQL_CA1_LOCK_UNLOCK,"lock_unlock"},
	{(unsigned int)SQL_CA1_POS_POSITION,"pos_position"},
	{(unsigned int)SQL_CA1_POS_UPDATE,"pos_update"},
	{(unsigned int)SQL_CA1_POS_DELETE,"pos_delete"},
	{(unsigned int)SQL_CA1_POS_REFRESH,"pos_refresh"},
	{(unsigned int)SQL_CA1_POSITIONED_UPDATE,"positioned_update"},
	{(unsigned int)SQL_CA1_POSITIONED_DELETE,"positioned_delete"},
	{(unsigned int)SQL_CA1_SELECT_FOR_UPDATE,"select_for_update"},
	{(unsigned int)SQL_CA1_BULK_ADD,"bulk_add"},
	{(unsigned int)SQL_CA1_BULK_UPDATE_BY_BOOKMARK,"bulk_update_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_DELETE_BY_BOOKMARK,"bulk_delete_by_bookmark"},
	{(unsigned int)SQL_CA1_BULK_FETCH_BY_BOOKMARK,"bulk_fetch_by_bookmark"},
	{0,NULL},
};

Getinfo_values getinfo_values_static_cursor_attributes2[] = {
	{(unsigned int)SQL_CA2_READ_ONLY_CONCURRENCY,"read_only_concurrency"},
	{(unsigned int)SQL_CA2_LOCK_CONCURRENCY,"lock_concurrency"},
	{(unsigned int)SQL_CA2_OPT_ROWVER_CONCURRENCY,"opt_rowver_concurrency"},
	{(unsigned int)SQL_CA2_OPT_VALUES_CONCURRENCY,"opt_values_concurrency"},
	{(unsigned int)SQL_CA2_SENSITIVITY_ADDITIONS,"sensitivity_additions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_DELETIONS,"sensitivity_deletions"},
	{(unsigned int)SQL_CA2_SENSITIVITY_UPDATES,"sensitivity_updates"},
	{(unsigned int)SQL_CA2_MAX_ROWS_SELECT,"max_rows_select"},
	{(unsigned int)SQL_CA2_MAX_ROWS_INSERT,"max_rows_insert"},
	{(unsigned int)SQL_CA2_MAX_ROWS_DELETE,"max_rows_delete"},
	{(unsigned int)SQL_CA2_MAX_ROWS_UPDATE,"max_rows_update"},
	{(unsigned int)SQL_CA2_MAX_ROWS_CATALOG,"max_rows_catalog"},
	{(unsigned int)SQL_CA2_MAX_ROWS_AFFECTS_ALL,"max_rows_affects_all"},
	{(unsigned int)SQL_CA2_CRC_EXACT,"crc_exact"},
	{(unsigned int)SQL_CA2_CRC_APPROXIMATE,"crc_approximate"},
	{(unsigned int)SQL_CA2_SIMULATE_NON_UNIQUE,"simulate_non_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_TRY_UNIQUE,"simulate_try_unique"},
	{(unsigned int)SQL_CA2_SIMULATE_UNIQUE,"simulate_unique"},
	{0,NULL},
};

Getinfo_values getinfo_values_string_functions[] = {
	{(unsigned int)SQL_FN_STR_ASCII,"ascii"},
	{(unsigned int)SQL_FN_STR_BIT_LENGTH,"bit_length"},
	{(unsigned int)SQL_FN_STR_CHAR,"char"},
	{(unsigned int)SQL_FN_STR_CHAR_LENGTH,"char_length"},
	{(unsigned int)SQL_FN_STR_CHARACTER_LENGTH,"character_length"},
	{(unsigned int)SQL_FN_STR_CONCAT,"concat"},
	{(unsigned int)SQL_FN_STR_DIFFERENCE,"difference"},
	{(unsigned int)SQL_FN_STR_INSERT,"insert"},
	{(unsigned int)SQL_FN_STR_LCASE,"lcase"},
	{(unsigned int)SQL_FN_STR_LEFT,"left"},
	{(unsigned int)SQL_FN_STR_LENGTH,"length"},
	{(unsigned int)SQL_FN_STR_LOCATE,"locate"},
	{(unsigned int)SQL_FN_STR_LTRIM,"ltrim"},
	{(unsigned int)SQL_FN_STR_OCTET_LENGTH,"octet_length"},
	{(unsigned int)SQL_FN_STR_POSITION,"position"},
	{(unsigned int)SQL_FN_STR_REPEAT,"repeat"},
	{(unsigned int)SQL_FN_STR_REPLACE,"replace"},
	{(unsigned int)SQL_FN_STR_RIGHT,"right"},
	{(unsigned int)SQL_FN_STR_RTRIM,"rtrim"},
	{(unsigned int)SQL_FN_STR_SOUNDEX,"soundex"},
	{(unsigned int)SQL_FN_STR_SPACE,"space"},
	{(unsigned int)SQL_FN_STR_SUBSTRING,"substring"},
	{(unsigned int)SQL_FN_STR_UCASE,"ucase"},
	{0,NULL},
};

Getinfo_values getinfo_values_subqueries[] = {
	{(unsigned int)SQL_SQ_CORRELATED_SUBQUERIES,"correlated_subqueries"},
	{(unsigned int)SQL_SQ_COMPARISON,"comparison"},
	{(unsigned int)SQL_SQ_EXISTS,"exists"},
	{(unsigned int)SQL_SQ_IN,"in"},
	{(unsigned int)SQL_SQ_QUANTIFIED,"quantified"},
	{0,NULL},
};

Getinfo_values getinfo_values_system_functions[] = {
	{(unsigned int)SQL_FN_SYS_DBNAME,"dbname"},
	{(unsigned int)SQL_FN_SYS_IFNULL,"ifnull"},
	{(unsigned int)SQL_FN_SYS_USERNAME,"username"},
	{0,NULL},
};

Getinfo_values getinfo_values_timedate_add_intervals[] = {
	{(unsigned int)SQL_FN_TSI_FRAC_SECOND,"frac_second"},
	{(unsigned int)SQL_FN_TSI_SECOND,"second"},
	{(unsigned int)SQL_FN_TSI_MINUTE,"minute"},
	{(unsigned int)SQL_FN_TSI_HOUR,"hour"},
	{(unsigned int)SQL_FN_TSI_DAY,"day"},
	{(unsigned int)SQL_FN_TSI_WEEK,"week"},
	{(unsigned int)SQL_FN_TSI_MONTH,"month"},
	{(unsigned int)SQL_FN_TSI_QUARTER,"quarter"},
	{(unsigned int)SQL_FN_TSI_YEAR,"year"},
	{0,NULL},
};

Getinfo_values getinfo_values_timedate_diff_intervals[] = {
	{(unsigned int)SQL_FN_TSI_FRAC_SECOND,"frac_second"},
	{(unsigned int)SQL_FN_TSI_SECOND,"second"},
	{(unsigned int)SQL_FN_TSI_MINUTE,"minute"},
	{(unsigned int)SQL_FN_TSI_HOUR,"hour"},
	{(unsigned int)SQL_FN_TSI_DAY,"day"},
	{(unsigned int)SQL_FN_TSI_WEEK,"week"},
	{(unsigned int)SQL_FN_TSI_MONTH,"month"},
	{(unsigned int)SQL_FN_TSI_QUARTER,"quarter"},
	{(unsigned int)SQL_FN_TSI_YEAR,"year"},
	{0,NULL},
};

Getinfo_values getinfo_values_timedate_functions[] = {
	{(unsigned int)SQL_FN_TD_CURRENT_DATE,"current_date"},
	{(unsigned int)SQL_FN_TD_CURRENT_TIME,"current_time"},
	{(unsigned int)SQL_FN_TD_CURRENT_TIMESTAMP,"current_timestamp"},
	{(unsigned int)SQL_FN_TD_CURDATE,"curdate"},
	{(unsigned int)SQL_FN_TD_CURTIME,"curtime"},
	{(unsigned int)SQL_FN_TD_DAYNAME,"dayname"},
	{(unsigned int)SQL_FN_TD_DAYOFMONTH,"dayofmonth"},
	{(unsigned int)SQL_FN_TD_DAYOFWEEK,"dayofweek"},
	{(unsigned int)SQL_FN_TD_DAYOFYEAR,"dayofyear"},
	{(unsigned int)SQL_FN_TD_EXTRACT,"extract"},
	{(unsigned int)SQL_FN_TD_HOUR,"hour"},
	{(unsigned int)SQL_FN_TD_MINUTE,"minute"},
	{(unsigned int)SQL_FN_TD_MONTH,"month"},
	{(unsigned int)SQL_FN_TD_MONTHNAME,"monthname"},
	{(unsigned int)SQL_FN_TD_NOW,"now"},
	{(unsigned int)SQL_FN_TD_QUARTER,"quarter"},
	{(unsigned int)SQL_FN_TD_SECOND,"second"},
	{(unsigned int)SQL_FN_TD_TIMESTAMPADD,"timestampadd"},
	{(unsigned int)SQL_FN_TD_TIMESTAMPDIFF,"timestampdiff"},
	{(unsigned int)SQL_FN_TD_WEEK,"week"},
	{(unsigned int)SQL_FN_TD_YEAR,"year"},
	{0,NULL},
};

Getinfo_values getinfo_values_txn_capable[] = {
	{(unsigned int)SQL_TC_NONE,"none"},
	{(unsigned int)SQL_TC_DML,"dml"},
	{(unsigned int)SQL_TC_DDL_COMMIT,"ddl_commit"},
	{(unsigned int)SQL_TC_DDL_IGNORE,"ddl_ignore"},
	{(unsigned int)SQL_TC_ALL,"all"},
	{0,NULL},
};

Getinfo_values getinfo_values_txn_isolation_option[] = {
	{(unsigned int)SQL_TXN_READ_UNCOMMITTED,"read_uncommitted"},
	{(unsigned int)SQL_TXN_READ_COMMITTED,"read_committed"},
	{(unsigned int)SQL_TXN_REPEATABLE_READ,"repeatable_read"},
	{(unsigned int)SQL_TXN_SERIALIZABLE,"serializable"},
	{0,NULL},
};

Getinfo_values getinfo_values_union[] = {
	{(unsigned int)SQL_U_UNION,"union"},
	{(unsigned int)SQL_U_UNION_ALL,"union_all"},
	{0,NULL},
};

/* ---- Codes ---- */
Getinfo_cor getinfo_cor5[] = {
	{"union",SQL_UNION,UBITMASK,getinfo_values_union},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor6[] = {
	{"dm_ver",SQL_DM_VER,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor8[] = {
	{"dbms_ver",SQL_DBMS_VER,STRING,NULL},
	{"group_by",SQL_GROUP_BY,VALUE,getinfo_values_group_by},
	{"keywords",SQL_KEYWORDS,STRING,NULL},
	{"odbc_ver",SQL_ODBC_VER,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor9[] = {
	{"dbms_name",SQL_DBMS_NAME,STRING,NULL},
	{"ddl_index",SQL_DDL_INDEX,VALUE32,getinfo_values_ddl_index},
	{"drop_view",SQL_DROP_VIEW,UBITMASK,getinfo_values_drop_view},
	{"integrity",SQL_INTEGRITY,STRING,NULL},
	{"user_name",SQL_USER_NAME,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor10[] = {
	{"async_mode",SQL_ASYNC_MODE,VALUE32,getinfo_values_async_mode},
	{"driver_ver",SQL_DRIVER_VER,STRING,NULL},
	{"drop_table",SQL_DROP_TABLE,UBITMASK,getinfo_values_drop_table},
	{"file_usage",SQL_FILE_USAGE,VALUE,getinfo_values_file_usage},
	{"procedures",SQL_PROCEDURES,STRING,NULL},
	{"subqueries",SQL_SUBQUERIES,UBITMASK,getinfo_values_subqueries},
	{"table_term",SQL_TABLE_TERM,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor11[] = {
	{"alter_table",SQL_ALTER_TABLE,UBITMASK,getinfo_values_alter_table},
	{"convert_bit",SQL_CONVERT_BIT,UBITMASK,getinfo_values_convert_bigint},
	{"create_view",SQL_CREATE_VIEW,UBITMASK,getinfo_values_create_view},
	{"driver_hdbc",SQL_DRIVER_HDBC,VALUE32,NULL},
	{"driver_henv",SQL_DRIVER_HENV,VALUE32,NULL},
	{"driver_hlib",SQL_DRIVER_HLIB,VALUE32,NULL},
	{"driver_name",SQL_DRIVER_NAME,STRING,NULL},
	{"drop_domain",SQL_DROP_DOMAIN,UBITMASK,getinfo_values_drop_domain},
	{"drop_schema",SQL_DROP_SCHEMA,UBITMASK,getinfo_values_drop_schema},
	{"row_updates",SQL_ROW_UPDATES,STRING,NULL},
	{"schema_term",SQL_SCHEMA_TERM,STRING,NULL},
	{"server_name",SQL_SERVER_NAME,STRING,NULL},
	{"sql92_grant",SQL_SQL92_GRANT,UBITMASK,getinfo_values_sql92_grant},
	{"txn_capable",SQL_TXN_CAPABLE,VALUE,getinfo_values_txn_capable},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor12[] = {
	{"alter_domain",SQL_ALTER_DOMAIN,UBITMASK,getinfo_values_alter_domain},
	{"catalog_name",SQL_CATALOG_NAME,STRING,NULL},
	{"catalog_term",SQL_CATALOG_TERM,STRING,NULL},
	{"convert_char",SQL_CONVERT_CHAR,UBITMASK,getinfo_values_convert_bigint},
	{"convert_date",SQL_CONVERT_DATE,UBITMASK,getinfo_values_convert_bigint},
	{"convert_real",SQL_CONVERT_REAL,UBITMASK,getinfo_values_convert_bigint},
	{"convert_time",SQL_CONVERT_TIME,UBITMASK,getinfo_values_convert_bigint},
	{"create_table",SQL_CREATE_TABLE,UBITMASK,getinfo_values_create_table},
	{"max_row_size",SQL_MAX_ROW_SIZE,VALUE32,NULL},
	{"schema_usage",SQL_SCHEMA_USAGE,UBITMASK,getinfo_values_schema_usage},
	{"sql92_revoke",SQL_SQL92_REVOKE,UBITMASK,getinfo_values_sql92_revoke},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor13[] = {
	{"batch_support",SQL_BATCH_SUPPORT,UBITMASK,getinfo_values_batch_support},
	{"catalog_usage",SQL_CATALOG_USAGE,UBITMASK,getinfo_values_catalog_usage},
	{"collation_seq",SQL_COLLATION_SEQ,STRING,NULL},
	{"convert_float",SQL_CONVERT_FLOAT,UBITMASK,getinfo_values_convert_bigint},
	{"create_domain",SQL_CREATE_DOMAIN,UBITMASK,getinfo_values_create_domain},
	{"create_schema",SQL_CREATE_SCHEMA,UBITMASK,getinfo_values_create_schema},
	{"database_name",SQL_DATABASE_NAME,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor14[] = {
	{"convert_bigint",SQL_CONVERT_BIGINT,UBITMASK,getinfo_values_convert_bigint},
	{"convert_binary",SQL_CONVERT_BINARY,UBITMASK,getinfo_values_convert_bigint},
	{"convert_double",SQL_CONVERT_DOUBLE,UBITMASK,getinfo_values_convert_bigint},
	{"drop_assertion",SQL_DROP_ASSERTION,UBITMASK,getinfo_values_drop_assertion},
	{"drop_collation",SQL_DROP_COLLATION,UBITMASK,getinfo_values_drop_collation},
	{"max_index_size",SQL_MAX_INDEX_SIZE,VALUE32,NULL},
	{"null_collation",SQL_NULL_COLLATION,VALUE,getinfo_values_null_collation},
	{"procedure_term",SQL_PROCEDURE_TERM,STRING,NULL},
	{"pos_operations",SQL_POS_OPERATIONS,BITMASK,getinfo_values_pos_operations},
	{"scroll_options",SQL_SCROLL_OPTIONS,UBITMASK,getinfo_values_scroll_options},
	{"xopen_cli_year",SQL_XOPEN_CLI_YEAR,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor15[] = {
	{"batch_row_count",SQL_BATCH_ROW_COUNT,UBITMASK,getinfo_values_batch_row_count},
	{"convert_decimal",SQL_CONVERT_DECIMAL,UBITMASK,getinfo_values_convert_bigint},
	{"convert_integer",SQL_CONVERT_INTEGER,UBITMASK,getinfo_values_convert_bigint},
	{"convert_numeric",SQL_CONVERT_NUMERIC,UBITMASK,getinfo_values_convert_bigint},
	{"convert_tinyint",SQL_CONVERT_TINYINT,UBITMASK,getinfo_values_convert_bigint},
	{"convert_varchar",SQL_CONVERT_VARCHAR,UBITMASK,getinfo_values_convert_bigint},
	{"driver_odbc_ver",SQL_DRIVER_ODBC_VER,STRING,NULL},
	{"identifier_case",SQL_IDENTIFIER_CASE,VALUE,getinfo_values_identifier_case},
	{"oj_capabilities",SQL_OJ_CAPABILITIES,UBITMASK,getinfo_values_oj_capabilities},
	{"sql_conformance",SQL_SQL_CONFORMANCE,VALUE32,getinfo_values_sql_conformance},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor16[] = {
	{"catalog_location",SQL_CATALOG_LOCATION,VALUE,getinfo_values_catalog_location},
	{"convert_smallint",SQL_CONVERT_SMALLINT,UBITMASK,getinfo_values_convert_bigint},
	{"correlation_name",SQL_CORRELATION_NAME,VALUE,getinfo_values_correlation_name},
	{"create_assertion",SQL_CREATE_ASSERTION,UBITMASK,getinfo_values_create_assertion},
	{"create_collation",SQL_CREATE_COLLATION,UBITMASK,getinfo_values_create_collation},
	{"data_source_name",SQL_DATA_SOURCE_NAME,STRING,NULL},
	{"drop_translation",SQL_DROP_TRANSLATION,UBITMASK,getinfo_values_drop_translation},
	{"insert_statement",SQL_INSERT_STATEMENT,UBITMASK,getinfo_values_insert_statement},
	{"mult_result_sets",SQL_MULT_RESULT_SETS,STRING,NULL},
	{"sql92_predicates",SQL_SQL92_PREDICATES,UBITMASK,getinfo_values_sql92_predicates},
	{"string_functions",SQL_STRING_FUNCTIONS,UBITMASK,getinfo_values_string_functions},
	{"system_functions",SQL_SYSTEM_FUNCTIONS,UBITMASK,getinfo_values_system_functions},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor17[] = {
	{"accessible_tables",SQL_ACCESSIBLE_TABLES,STRING,NULL},
	{"convert_timestamp",SQL_CONVERT_TIMESTAMP,UBITMASK,getinfo_values_convert_bigint},
	{"convert_varbinary",SQL_CONVERT_VARBINARY,UBITMASK,getinfo_values_convert_bigint},
	{"convert_functions",SQL_CONVERT_FUNCTIONS,UBITMASK,getinfo_values_convert_functions},
	{"datetime_literals",SQL_DATETIME_LITERALS,UBITMASK,getinfo_values_datetime_literals},
	{"info_schema_views",SQL_INFO_SCHEMA_VIEWS,UBITMASK,getinfo_values_info_schema_views},
	{"max_statement_len",SQL_MAX_STATEMENT_LEN,VALUE32,NULL},
	{"max_user_name_len",SQL_MAX_USER_NAME_LEN,VALUE,NULL},
	{"numeric_functions",SQL_NUMERIC_FUNCTIONS,UBITMASK,getinfo_values_numeric_functions},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor18[] = {
	{"create_translation",SQL_CREATE_TRANSLATION,UBITMASK,getinfo_values_create_translation},
	{"describe_parameter",SQL_DESCRIBE_PARAMETER,STRING,NULL},
	{"drop_character_set",SQL_DROP_CHARACTER_SET,UBITMASK,getinfo_values_drop_character_set},
	{"getdata_extensions",SQL_GETDATA_EXTENSIONS,UBITMASK,getinfo_values_getdata_extensions},
	{"like_escape_clause",SQL_LIKE_ESCAPE_CLAUSE,STRING,NULL},
	{"max_identifier_len",SQL_MAX_IDENTIFIER_LEN,VALUE,NULL},
	{"max_table_name_len",SQL_MAX_TABLE_NAME_LEN,VALUE,NULL},
	{"need_long_data_len",SQL_NEED_LONG_DATA_LEN,STRING,NULL},
	{"special_characters",SQL_SPECIAL_CHARACTERS,STRING,NULL},
	{"timedate_functions",SQL_TIMEDATE_FUNCTIONS,UBITMASK,getinfo_values_timedate_functions},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor19[] = {
	{"active_environments",SQL_ACTIVE_ENVIRONMENTS,VALUE,NULL},
	{"aggregate_functions",SQL_AGGREGATE_FUNCTIONS,UBITMASK,getinfo_values_aggregate_functions},
	{"convert_longvarchar",SQL_CONVERT_LONGVARCHAR,UBITMASK,getinfo_values_convert_bigint},
	{"max_column_name_len",SQL_MAX_COLUMN_NAME_LEN,VALUE,NULL},
	{"max_cursor_name_len",SQL_MAX_CURSOR_NAME_LEN,VALUE,NULL},
	{"max_schema_name_len",SQL_MAX_SCHEMA_NAME_LEN,VALUE,NULL},
	{"multiple_active_txn",SQL_MULTIPLE_ACTIVE_TXN,STRING,NULL},
	{"param_array_selects",SQL_PARAM_ARRAY_SELECTS,ENUM,getinfo_values_param_array_selects},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor20[] = {
	{"bookmark_persistence",SQL_BOOKMARK_PERSISTENCE,UBITMASK,getinfo_values_bookmark_persistence},
	{"concat_null_behavior",SQL_CONCAT_NULL_BEHAVIOR,VALUE,getinfo_values_concat_null_behavior},
	{"create_character_set",SQL_CREATE_CHARACTER_SET,UBITMASK,getinfo_values_create_character_set},
	{"max_catalog_name_len",SQL_MAX_CATALOG_NAME_LEN,VALUE,NULL},
	{"max_char_literal_len",SQL_MAX_CHAR_LITERAL_LEN,VALUE32,NULL},
	{"max_columns_in_index",SQL_MAX_COLUMNS_IN_INDEX,VALUE,NULL},
	{"max_columns_in_table",SQL_MAX_COLUMNS_IN_TABLE,VALUE,NULL},
	{"max_tables_in_select",SQL_MAX_TABLES_IN_SELECT,VALUE,NULL},
	{"non_nullable_columns",SQL_NON_NULLABLE_COLUMNS,VALUE,getinfo_values_non_nullable_columns},
	{"txn_isolation_option",SQL_TXN_ISOLATION_OPTION,UBITMASK,getinfo_values_txn_isolation_option},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor21[] = {
	{"accessible_procedures",SQL_ACCESSIBLE_PROCEDURES,STRING,NULL},
	{"convert_longvarbinary",SQL_CONVERT_LONGVARBINARY,UBITMASK,getinfo_values_convert_bigint},
	{"data_source_read_only",SQL_DATA_SOURCE_READ_ONLY,STRING,NULL},
	{"default_txn_isolation",SQL_DEFAULT_TXN_ISOLATION,VALUE32,getinfo_values_default_txn_isolation},
	{"identifier_quote_char",SQL_IDENTIFIER_QUOTE_CHAR,UBITMASK,getinfo_values_identifier_quote_char},
	{"max_columns_in_select",SQL_MAX_COLUMNS_IN_SELECT,VALUE,NULL},
	{"search_pattern_escape",SQL_SEARCH_PATTERN_ESCAPE,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor22[] = {
	{"catalog_name_separator",SQL_CATALOG_NAME_SEPARATOR,STRING,NULL},
	{"cursor_commit_behavior",SQL_CURSOR_COMMIT_BEHAVIOR,VALUE,getinfo_values_cursor_commit_behavior},
	{"expressions_in_orderby",SQL_EXPRESSIONS_IN_ORDERBY,STRING,NULL},
	{"max_binary_literal_len",SQL_MAX_BINARY_LITERAL_LEN,VALUE32,NULL},
	{"max_driver_connections",SQL_MAX_DRIVER_CONNECTIONS,VALUE,NULL},
	{"max_procedure_name_len",SQL_MAX_PROCEDURE_NAME_LEN,VALUE,NULL},
	{"param_array_row_counts",SQL_PARAM_ARRAY_ROW_COUNTS,ENUM,getinfo_values_param_array_row_counts},
	{"quoted_identifier_case",SQL_QUOTED_IDENTIFIER_CASE,VALUE,getinfo_values_quoted_identifier_case},
	{"sql92_string_functions",SQL_SQL92_STRING_FUNCTIONS,UBITMASK,getinfo_values_sql92_string_functions},
	{"timedate_add_intervals",SQL_TIMEDATE_ADD_INTERVALS,UBITMASK,getinfo_values_timedate_add_intervals},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor23[] = {
	{"max_columns_in_group_by",SQL_MAX_COLUMNS_IN_GROUP_BY,VALUE,NULL},
	{"max_columns_in_order_by",SQL_MAX_COLUMNS_IN_ORDER_BY,VALUE,NULL},
	{"sql92_value_expressions",SQL_SQL92_VALUE_EXPRESSIONS,UBITMASK,getinfo_values_sql92_value_expressions},
	{"timedate_diff_intervals",SQL_TIMEDATE_DIFF_INTERVALS,UBITMASK,getinfo_values_timedate_diff_intervals},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor24[] = {
	{"cursor_rollback_behavior",SQL_CURSOR_ROLLBACK_BEHAVIOR,VALUE,getinfo_values_cursor_rollback_behavior},
	{"sql92_datetime_functions",SQL_SQL92_DATETIME_FUNCTIONS,UBITMASK,getinfo_values_sql92_datetime_functions},
	{"standard_cli_conformance",SQL_STANDARD_CLI_CONFORMANCE,UBITMASK,getinfo_values_standard_cli_conformance},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor25[] = {
	{"convert_interval_day_time",SQL_CONVERT_INTERVAL_DAY_TIME,UBITMASK,getinfo_values_convert_bigint},
	{"keyset_cursor_attributes1",SQL_KEYSET_CURSOR_ATTRIBUTES1,UBITMASK,getinfo_values_keyset_cursor_attributes1},
	{"keyset_cursor_attributes2",SQL_KEYSET_CURSOR_ATTRIBUTES2,UBITMASK,getinfo_values_keyset_cursor_attributes2},
	{"max_concurrent_activities",SQL_MAX_CONCURRENT_ACTIVITIES,VALUE,NULL},
	{"static_cursor_attributes1",SQL_STATIC_CURSOR_ATTRIBUTES1,UBITMASK,getinfo_values_static_cursor_attributes1},
	{"static_cursor_attributes2",SQL_STATIC_CURSOR_ATTRIBUTES2,UBITMASK,getinfo_values_static_cursor_attributes2},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor26[] = {
	{"dynamic_cursor_attributes1",SQL_DYNAMIC_CURSOR_ATTRIBUTES1,UBITMASK,getinfo_values_dynamic_cursor_attributes1},
	{"dynamic_cursor_attributes2",SQL_DYNAMIC_CURSOR_ATTRIBUTES2,UBITMASK,getinfo_values_dynamic_cursor_attributes2},
	{"max_row_size_includes_long",SQL_MAX_ROW_SIZE_INCLUDES_LONG,STRING,NULL},
	{"odbc_interface_conformance",SQL_ODBC_INTERFACE_CONFORMANCE,VALUE32,getinfo_values_odbc_interface_conformance},
	{"order_by_columns_in_select",SQL_ORDER_BY_COLUMNS_IN_SELECT,STRING,NULL},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor27[] = {
	{"convert_interval_year_month",SQL_CONVERT_INTERVAL_YEAR_MONTH,UBITMASK,getinfo_values_convert_bigint},
	{"sql92_row_value_constructor",SQL_SQL92_ROW_VALUE_CONSTRUCTOR,UBITMASK,getinfo_values_sql92_row_value_constructor},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor29[] = {
	{"sql92_foreign_key_delete_rule",SQL_SQL92_FOREIGN_KEY_DELETE_RULE,UBITMASK,getinfo_values_sql92_foreign_key_delete_rule},
	{"sql92_foreign_key_update_rule",SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,UBITMASK,getinfo_values_sql92_foreign_key_update_rule},
	{"sql92_numeric_value_functions",SQL_SQL92_NUMERIC_VALUE_FUNCTIONS,UBITMASK,getinfo_values_sql92_numeric_value_functions},

	{NULL,0,0,NULL}
};
Getinfo_cor getinfo_cor31[] = {
	{"forward_only_cursor_attributes1",SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1,UBITMASK,getinfo_values_forward_only_cursor_attributes1},
	{"forward_only_cursor_attributes2",SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2,UBITMASK,getinfo_values_forward_only_cursor_attributes2},
	{"max_async_concurrent_statements",SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,VALUE32,NULL},
	{"sql92_relational_join_operators",SQL_SQL92_RELATIONAL_JOIN_OPERATORS,UBITMASK,getinfo_values_sql92_relational_join_operators},

	{NULL,0,0,NULL}
};

Getinfo_cor *getinfo_cor[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	getinfo_cor5,
	getinfo_cor6,
	NULL,
	getinfo_cor8,
	getinfo_cor9,
	getinfo_cor10,
	getinfo_cor11,
	getinfo_cor12,
	getinfo_cor13,
	getinfo_cor14,
	getinfo_cor15,
	getinfo_cor16,
	getinfo_cor17,
	getinfo_cor18,
	getinfo_cor19,
	getinfo_cor20,
	getinfo_cor21,
	getinfo_cor22,
	getinfo_cor23,
	getinfo_cor24,
	getinfo_cor25,
	getinfo_cor26,
	getinfo_cor27,
	NULL,
	getinfo_cor29,
	NULL,
	getinfo_cor31,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

int getinfo_size = 38;

